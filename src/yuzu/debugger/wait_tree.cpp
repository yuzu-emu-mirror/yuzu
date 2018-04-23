// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "yuzu/debugger/wait_tree.h"
#include "yuzu/util/util.h"

#include "core/core.h"
#include "core/hle/kernel/event.h"
#include "core/hle/kernel/handle_table.h"
#include "core/hle/kernel/mutex.h"
#include "core/hle/kernel/thread.h"
#include "core/hle/kernel/timer.h"
#include "core/hle/kernel/wait_object.h"

WaitTreeItem::~WaitTreeItem() {}

QColor WaitTreeItem::GetColor() const {
    return QColor(Qt::GlobalColor::black);
}

std::vector<std::unique_ptr<WaitTreeItem>> WaitTreeItem::GetChildren() const {
    return {};
}

void WaitTreeItem::Expand() {
    if (IsExpandable() && !expanded) {
        children = GetChildren();
        for (std::size_t i = 0; i < children.size(); ++i) {
            children[i]->parent = this;
            children[i]->row = i;
        }
        expanded = true;
    }
}

WaitTreeItem* WaitTreeItem::Parent() const {
    return parent;
}

const std::vector<std::unique_ptr<WaitTreeItem>>& WaitTreeItem::Children() const {
    return children;
}

bool WaitTreeItem::IsExpandable() const {
    return false;
}

std::size_t WaitTreeItem::Row() const {
    return row;
}

std::vector<std::unique_ptr<WaitTreeThread>> WaitTreeItem::MakeThreadItemList() {
    const auto& threads = Core::System::GetInstance().Scheduler().GetThreadList();
    std::vector<std::unique_ptr<WaitTreeThread>> item_list;
    item_list.reserve(threads.size());
    for (std::size_t i = 0; i < threads.size(); ++i) {
        item_list.push_back(std::make_unique<WaitTreeThread>(*threads[i]));
        item_list.back()->row = i;
    }
    return item_list;
}

WaitTreeText::WaitTreeText(const QString& t) : text(t) {}

QString WaitTreeText::GetText() const {
    return text;
}

WaitTreeMutexInfo::WaitTreeMutexInfo(VAddr mutex_address) : mutex_address(mutex_address) {
    mutex_value = Memory::Read32(mutex_address);
    owner_handle = static_cast<Kernel::Handle>(mutex_value & Kernel::Mutex::MutexOwnerMask);
    owner = Kernel::g_handle_table.Get<Kernel::Thread>(owner_handle);
}

QString WaitTreeMutexInfo::GetText() const {
    return tr("waiting for mutex 0x%1").arg(mutex_address, 16, 16, QLatin1Char('0'));
}

std::vector<std::unique_ptr<WaitTreeItem>> WaitTreeMutexInfo::GetChildren() const {
    std::vector<std::unique_ptr<WaitTreeItem>> list;

    bool has_waiters = (mutex_value & Kernel::Mutex::MutexHasWaitersFlag) != 0;

    list.push_back(std::make_unique<WaitTreeText>(tr("has waiters: %1").arg(has_waiters)));
    list.push_back(std::make_unique<WaitTreeText>(
        tr("owner handle: 0x%1").arg(owner_handle, 8, 16, QLatin1Char('0'))));
    if (owner != nullptr)
        list.push_back(std::make_unique<WaitTreeThread>(*owner));
    return list;
}

WaitTreeWaitObject::WaitTreeWaitObject(const Kernel::WaitObject& o) : object(o) {}

bool WaitTreeExpandableItem::IsExpandable() const {
    return true;
}

QString WaitTreeWaitObject::GetText() const {
    return tr("[%1]%2 %3")
        .arg(object.GetObjectId())
        .arg(QString::fromStdString(object.GetTypeName()),
             QString::fromStdString(object.GetName()));
}

std::unique_ptr<WaitTreeWaitObject> WaitTreeWaitObject::make(const Kernel::WaitObject& object) {
    switch (object.GetHandleType()) {
    case Kernel::HandleType::Event:
        return std::make_unique<WaitTreeEvent>(static_cast<const Kernel::Event&>(object));
    case Kernel::HandleType::Timer:
        return std::make_unique<WaitTreeTimer>(static_cast<const Kernel::Timer&>(object));
    case Kernel::HandleType::Thread:
        return std::make_unique<WaitTreeThread>(static_cast<const Kernel::Thread&>(object));
    default:
        return std::make_unique<WaitTreeWaitObject>(object);
    }
}

std::vector<std::unique_ptr<WaitTreeItem>> WaitTreeWaitObject::GetChildren() const {
    std::vector<std::unique_ptr<WaitTreeItem>> list;

    const auto& threads = object.GetWaitingThreads();
    if (threads.empty()) {
        list.push_back(std::make_unique<WaitTreeText>(tr("waited by no thread")));
    } else {
        list.push_back(std::make_unique<WaitTreeThreadList>(threads));
    }
    return list;
}

QString WaitTreeWaitObject::GetResetTypeQString(Kernel::ResetType reset_type) {
    switch (reset_type) {
    case Kernel::ResetType::OneShot:
        return tr("one shot");
    case Kernel::ResetType::Sticky:
        return tr("sticky");
    case Kernel::ResetType::Pulse:
        return tr("pulse");
    }
}

WaitTreeObjectList::WaitTreeObjectList(
    const std::vector<Kernel::SharedPtr<Kernel::WaitObject>>& list, bool w_all)
    : object_list(list), wait_all(w_all) {}

QString WaitTreeObjectList::GetText() const {
    if (wait_all)
        return tr("waiting for all objects");
    return tr("waiting for one of the following objects");
}

std::vector<std::unique_ptr<WaitTreeItem>> WaitTreeObjectList::GetChildren() const {
    std::vector<std::unique_ptr<WaitTreeItem>> list(object_list.size());
    std::transform(object_list.begin(), object_list.end(), list.begin(),
                   [](const auto& t) { return WaitTreeWaitObject::make(*t); });
    return list;
}

WaitTreeThread::WaitTreeThread(const Kernel::Thread& thread) : WaitTreeWaitObject(thread) {}

QString WaitTreeThread::GetText() const {
    const auto& thread = static_cast<const Kernel::Thread&>(object);
    QString status;
    switch (thread.status) {
    case THREADSTATUS_RUNNING:
        status = tr("running");
        break;
    case THREADSTATUS_READY:
        status = tr("ready");
        break;
    case THREADSTATUS_WAIT_HLE_EVENT:
        status = tr("waiting for HLE return");
        break;
    case THREADSTATUS_WAIT_SLEEP:
        status = tr("sleeping");
        break;
    case THREADSTATUS_WAIT_SYNCH_ALL:
    case THREADSTATUS_WAIT_SYNCH_ANY:
        status = tr("waiting for objects");
        break;
    case THREADSTATUS_WAIT_MUTEX:
        status = tr("waiting for mutex");
        break;
    case THREADSTATUS_DORMANT:
        status = tr("dormant");
        break;
    case THREADSTATUS_DEAD:
        status = tr("dead");
        break;
    }
    QString pc_info = tr(" PC = 0x%1 LR = 0x%2")
                          .arg(thread.context.pc, 8, 16, QLatin1Char('0'))
                          .arg(thread.context.cpu_registers[30], 8, 16, QLatin1Char('0'));
    return WaitTreeWaitObject::GetText() + pc_info + " (" + status + ") ";
}

QColor WaitTreeThread::GetColor() const {
    const auto& thread = static_cast<const Kernel::Thread&>(object);
    switch (thread.status) {
    case THREADSTATUS_RUNNING:
        return QColor(Qt::GlobalColor::darkGreen);
    case THREADSTATUS_READY:
        return QColor(Qt::GlobalColor::darkBlue);
    case THREADSTATUS_WAIT_HLE_EVENT:
        return QColor(Qt::GlobalColor::darkRed);
    case THREADSTATUS_WAIT_SLEEP:
        return QColor(Qt::GlobalColor::darkYellow);
    case THREADSTATUS_WAIT_SYNCH_ALL:
    case THREADSTATUS_WAIT_SYNCH_ANY:
    case THREADSTATUS_WAIT_MUTEX:
        return QColor(Qt::GlobalColor::red);
    case THREADSTATUS_DORMANT:
        return QColor(Qt::GlobalColor::darkCyan);
    case THREADSTATUS_DEAD:
        return QColor(Qt::GlobalColor::gray);
    default:
        return WaitTreeItem::GetColor();
    }
}

std::vector<std::unique_ptr<WaitTreeItem>> WaitTreeThread::GetChildren() const {
    std::vector<std::unique_ptr<WaitTreeItem>> list(WaitTreeWaitObject::GetChildren());

    const auto& thread = static_cast<const Kernel::Thread&>(object);

    QString processor;
    switch (thread.processor_id) {
    case ThreadProcessorId::THREADPROCESSORID_DEFAULT:
        processor = tr("default");
        break;
    case ThreadProcessorId::THREADPROCESSORID_0:
    case ThreadProcessorId::THREADPROCESSORID_1:
    case ThreadProcessorId::THREADPROCESSORID_2:
    case ThreadProcessorId::THREADPROCESSORID_3:
        processor = tr("core %1").arg(thread.processor_id);
        break;
    default:
        processor = tr("Unknown processor %1").arg(thread.processor_id);
        break;
    }

    list.push_back(std::make_unique<WaitTreeText>(tr("processor = %1").arg(processor)));
    list.push_back(std::make_unique<WaitTreeText>(tr("thread id = %1").arg(thread.GetThreadId())));
    list.push_back(std::make_unique<WaitTreeText>(tr("priority = %1(current) / %2(normal)")
                                                      .arg(thread.current_priority)
                                                      .arg(thread.nominal_priority)));
    list.push_back(std::make_unique<WaitTreeText>(
        tr("last running ticks = %1").arg(thread.last_running_ticks)));

    if (thread.mutex_wait_address != 0)
        list.push_back(std::make_unique<WaitTreeMutexInfo>(thread.mutex_wait_address));
    else
        list.push_back(std::make_unique<WaitTreeText>(tr("not waiting for mutex")));

    if (thread.status == THREADSTATUS_WAIT_SYNCH_ANY ||
        thread.status == THREADSTATUS_WAIT_SYNCH_ALL) {
        list.push_back(std::make_unique<WaitTreeObjectList>(thread.wait_objects,
                                                            thread.IsSleepingOnWaitAll()));
    }

    return list;
}

WaitTreeEvent::WaitTreeEvent(const Kernel::Event& object) : WaitTreeWaitObject(object) {}

std::vector<std::unique_ptr<WaitTreeItem>> WaitTreeEvent::GetChildren() const {
    std::vector<std::unique_ptr<WaitTreeItem>> list(WaitTreeWaitObject::GetChildren());

    list.push_back(std::make_unique<WaitTreeText>(
        tr("reset type = %1")
            .arg(GetResetTypeQString(static_cast<const Kernel::Event&>(object).reset_type))));
    return list;
}

WaitTreeTimer::WaitTreeTimer(const Kernel::Timer& object) : WaitTreeWaitObject(object) {}

std::vector<std::unique_ptr<WaitTreeItem>> WaitTreeTimer::GetChildren() const {
    std::vector<std::unique_ptr<WaitTreeItem>> list(WaitTreeWaitObject::GetChildren());

    const auto& timer = static_cast<const Kernel::Timer&>(object);

    list.push_back(std::make_unique<WaitTreeText>(
        tr("reset type = %1").arg(GetResetTypeQString(timer.reset_type))));
    list.push_back(
        std::make_unique<WaitTreeText>(tr("initial delay = %1").arg(timer.initial_delay)));
    list.push_back(
        std::make_unique<WaitTreeText>(tr("interval delay = %1").arg(timer.interval_delay)));
    return list;
}

WaitTreeThreadList::WaitTreeThreadList(const std::vector<Kernel::SharedPtr<Kernel::Thread>>& list)
    : thread_list(list) {}

QString WaitTreeThreadList::GetText() const {
    return tr("waited by thread");
}

std::vector<std::unique_ptr<WaitTreeItem>> WaitTreeThreadList::GetChildren() const {
    std::vector<std::unique_ptr<WaitTreeItem>> list(thread_list.size());
    std::transform(thread_list.begin(), thread_list.end(), list.begin(),
                   [](const auto& t) { return std::make_unique<WaitTreeThread>(*t); });
    return list;
}

WaitTreeModel::WaitTreeModel(QObject* parent) : QAbstractItemModel(parent) {}

QModelIndex WaitTreeModel::index(int row, int column, const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent))
        return {};

    if (parent.isValid()) {
        WaitTreeItem* parent_item = static_cast<WaitTreeItem*>(parent.internalPointer());
        parent_item->Expand();
        return createIndex(row, column, parent_item->Children()[row].get());
    }

    return createIndex(row, column, thread_items[row].get());
}

QModelIndex WaitTreeModel::parent(const QModelIndex& index) const {
    if (!index.isValid())
        return {};

    WaitTreeItem* parent_item = static_cast<WaitTreeItem*>(index.internalPointer())->Parent();
    if (!parent_item) {
        return QModelIndex();
    }
    return createIndex(static_cast<int>(parent_item->Row()), 0, parent_item);
}

int WaitTreeModel::rowCount(const QModelIndex& parent) const {
    if (!parent.isValid())
        return static_cast<int>(thread_items.size());

    WaitTreeItem* parent_item = static_cast<WaitTreeItem*>(parent.internalPointer());
    parent_item->Expand();
    return static_cast<int>(parent_item->Children().size());
}

int WaitTreeModel::columnCount(const QModelIndex&) const {
    return 1;
}

QVariant WaitTreeModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid())
        return {};

    switch (role) {
    case Qt::DisplayRole:
        return static_cast<WaitTreeItem*>(index.internalPointer())->GetText();
    case Qt::ForegroundRole:
        return static_cast<WaitTreeItem*>(index.internalPointer())->GetColor();
    default:
        return {};
    }
}

void WaitTreeModel::ClearItems() {
    thread_items.clear();
}

void WaitTreeModel::InitItems() {
    thread_items = WaitTreeItem::MakeThreadItemList();
}

WaitTreeWidget::WaitTreeWidget(QWidget* parent) : QDockWidget(tr("Wait Tree"), parent) {
    setObjectName("WaitTreeWidget");
    view = new QTreeView(this);
    view->setHeaderHidden(true);
    setWidget(view);
    setEnabled(false);
}

void WaitTreeWidget::OnDebugModeEntered() {
    if (!Core::System::GetInstance().IsPoweredOn())
        return;
    model->InitItems();
    view->setModel(model);
    setEnabled(true);
}

void WaitTreeWidget::OnDebugModeLeft() {
    setEnabled(false);
    view->setModel(nullptr);
    model->ClearItems();
}

void WaitTreeWidget::OnEmulationStarting(EmuThread* emu_thread) {
    model = new WaitTreeModel(this);
    view->setModel(model);
    setEnabled(false);
}

void WaitTreeWidget::OnEmulationStopping() {
    view->setModel(nullptr);
    delete model;
    setEnabled(false);
}
