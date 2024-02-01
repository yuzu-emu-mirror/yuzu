// SPDX-FileCopyrightText: 2023 Yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QEventLoop>
#include <QHttpMultiPart>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QUrl>

#include "common/logging/log.h"
#include "yuzu/compatdb.h"

QString intro_login = QString::fromLatin1("intro.login");
QByteArray set_cookie = QByteArray{"Set-Cookie"};
QString logout_token = QString::fromLatin1("logout_token");
QString csrf_token = QString::fromLatin1("csrf_token");
QString content_json = QString::fromLatin1("application/json");
QString boot_yes = QString::fromLatin1("boot.yes");
QString boot_no = QString::fromLatin1("boot.no");
QString play_yes = QString::fromLatin1("play.yes");
QString play_no = QString::fromLatin1("play.no");
QString freeze_yes_ = QString::fromLatin1("freeze.yes");
QString freeze_no_ = QString::fromLatin1("freeze.no");
QString complete_yes_ = QString::fromLatin1("complete.yes");
QString complete_no_ = QString::fromLatin1("complete.no");
QString graphical_major_ = QString::fromLatin1("graphical.major");
QString graphical_minor_ = QString::fromLatin1("graphical.minor");
QString graphical_none_ = QString::fromLatin1("graphical.none");
QString audio_major_ = QString::fromLatin1("audio.major");
QString audio_minor_ = QString::fromLatin1("audio.minor");
QString audio_none_ = QString::fromLatin1("audio.none");
QString comment_text = QString::fromLatin1("comment.text");
QString comment_use = QString::fromLatin1("comment.use");
QString screenshot_yes_ = QString::fromLatin1("screenshot.yes");
QString screenshot_no_ = QString::fromLatin1("screenshot.no");

std::string ADD = "/entity/yuzu_report?_format=json";
std::string EDIT = "/yuzu-reports/";
std::string COOKIE = "Cookie";
std::string CSRF_TOKEN = "X-CSRF-Token";
std::string LOGIN = "/yuzu-login/hrrr";
std::string LOGOUT = "/user/logout?_format=json&token=";
std::string REPORT_ID = "x-report-id";
std::string TOKEN = "/session/token";
std::string DCU = "/yuzu-uploads/dcu";

IntroPage::IntroPage(std::string* host_, std::string* username_, std::string* token_,
                     std::string* cookie_, std::string* current_user_,
                     QNetworkAccessManager& network_manager_, QWidget* parent)
    : QWizardPage(parent), host{host_}, username{username_}, token{token_}, cookie{cookie_},
      current_user{current_user_}, network_manager{network_manager_} {
    setTitle(tr("Welcome"));
    setCommitPage(true);
    setButtonText(QWizard::CommitButton, tr("Start"));

    header = new QLabel(
        tr("<html>"
           "<head/>"
           "<body>"
           "<p>"
           "<span style=\" font-size:10pt;\">"
           "Should you choose to submit a test case to the "
           "</span>"
           "<a href=\"https://yuzu-emu.org/game/\">"
           "<span style=\" font-size:10pt; text-decoration: underline; color:#0AB9E6;\">"
           "Yuzu Compatibility List."
           "</span>"
           "</a>"
           "</p>"
           "<p style=\" font-size:10pt;\">"
           "The following information will be collected and displayed on the site:"
           "</p>"
           "<ul style=\"margin-top: 0px; margin-bottom: 0px; margin-left: 0px; margin-right: 0px; "
           "-qt-list-indent: 1;\">"
           "<li style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; "
           "-qt-block-indent:0; text-indent:0px;\">"
           "Hardware Information (CPU / GPU / Operating System)"
           "</li>"
           "<li style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; "
           "-qt-block-indent:0; text-indent:0px;\">"
           "Which version of yuzu you are running"
           "</li>"
           "<li style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; "
           "-qt-block-indent:0; text-indent:0px;\">"
           "The connected yuzu account"
           "</li>"
           "</ul>"
           "</body>"
           "</html>"));
    header->setTextFormat(Qt::RichText);
    header->setWordWrap(true);

    login = tr("Login");
    logout = tr("Logout");
    ready = tr("Ready");

    cookie_verified = new QLabel();
    report_login = new QPushButton(login);
    report_logout = new QPushButton(logout);
    report_logout->setVisible(false);
    login_yes = new QRadioButton(this);
    login_yes->setVisible(false);

    h_layout = new QHBoxLayout();
    h_layout->addWidget(report_login);
    h_layout->addWidget(cookie_verified);
    h_layout->addWidget(report_logout);

    layout = new QVBoxLayout();
    layout->addWidget(header);
    layout->addItem(h_layout);
    setLayout(layout);

    registerField(intro_login, login_yes);

    if (!cookie->empty()) {
        report_login->setText(ready);
        report_login->setDisabled(true);
        report_logout->setVisible(true);
        login_yes->setChecked(true);
    }

    connect(report_login, &QPushButton::clicked, this, &IntroPage::Login);
    connect(report_logout, &QPushButton::clicked, this, &IntroPage::Logout);
    connect(login_yes, &QRadioButton::toggled, this, &QWizardPage::completeChanged);
}

bool IntroPage::isComplete() const {
    return login_yes->isChecked();
}

void IntroPage::OnLogin(QNetworkReply* reply) {
    network_manager.disconnect(SIGNAL(finished(QNetworkReply*)));
    if (reply->error() != QNetworkReply::NoError) {
        emit NetworkError(reply->errorString());
        report_login->setDisabled(false);
        report_login->setText(login);
        LOG_WARNING(Frontend, "Login Failed: {}:{}", reply->errorString().toStdString(),
                    reply->readAll().constData());
        return;
    }

    if (reply->hasRawHeader(set_cookie)) {
        auto str_reply = reply->readAll().toStdString();
        *current_user = str_reply;
        *cookie = reply->rawHeader(set_cookie).toStdString();
        emit UserChange();
        report_login->setText(ready);
        report_login->setDisabled(true);
        cookie_verified->setPixmap(QIcon::fromTheme(QStringLiteral("checked")).pixmap(16));
        cookie_verified->setToolTip(tr("Verified", "Tooltip"));
        login_yes->setChecked(true);
        report_logout->setVisible(true);
    } else {
        report_login->setText(login);
        cookie_verified->setPixmap(QIcon::fromTheme(QStringLiteral("failed")).pixmap(16));
        cookie_verified->setToolTip(tr("Verification Failed", "Tooltip"));
        report_login->setDisabled(false);
    }
}

void IntroPage::OnLogout(QNetworkReply* reply) {
    network_manager.disconnect(SIGNAL(finished(QNetworkReply*)));
    if (reply->error() != QNetworkReply::NoError) {
        emit NetworkError(reply->errorString());
        LOG_WARNING(Frontend, "Logout Failed: {}:{}", reply->errorString().toStdString(),
                    reply->readAll().constData());
    }

    *current_user = "";
    *cookie = "";
    emit UserChange();
    report_logout->setVisible(false);
    report_logout->setText(logout);
    report_login->setText(login);
    report_login->setDisabled(false);
    cookie_verified->clear();
    login_yes->setChecked(false);
    emit LoggedOut();
}

void IntroPage::Logout() {
    QJsonDocument json_user = QJsonDocument::fromJson(QByteArray::fromStdString(*current_user));
    QJsonObject json_obj = json_user.object();
    std::string l_token = json_obj.value(logout_token).toString().toStdString();
    std::string c_token = json_obj.value(csrf_token).toString().toStdString();

    report_logout->setDisabled(true);
    report_logout->setText(tr("Logging out..."));

    auto url = *host + LOGOUT + l_token;
    connect(&network_manager, &QNetworkAccessManager::finished,
            [this](QNetworkReply* reply) { OnLogout(reply); });

    QNetworkRequest request;
    request.setUrl(QUrl(QString::fromStdString(url)));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QVariant::fromValue<QString>(content_json));
    request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
    request.setRawHeader(COOKIE.c_str(), cookie->c_str());
    request.setRawHeader(CSRF_TOKEN.c_str(), c_token.c_str());
    request.setTransferTimeout(3000);
    request.setAttribute(QNetworkRequest::AutoDeleteReplyOnFinishAttribute, true);
    network_manager.post(request, "");
}

void IntroPage::Login() {
    emit ClearErrors();
    report_login->setDisabled(true);
    report_login->setText(tr("Verifying..."));
    cookie_verified->setPixmap(QIcon::fromTheme(QStringLiteral("sync")).pixmap(16));

    connect(&network_manager, &QNetworkAccessManager::finished,
            [this](QNetworkReply* reply) { OnLogin(reply); });

    QNetworkRequest request;
    request.setUrl(QUrl(QString::fromStdString(*host + LOGIN)));
    request.setTransferTimeout(3000);
    request.setAttribute(QNetworkRequest::AutoDeleteReplyOnFinishAttribute, true);
    request.setRawHeader("x-username", username->c_str());
    request.setRawHeader("x-token", token->c_str());
    request.setRawHeader("api-verison", "1");
    network_manager.get(request);
}

BootPage::BootPage(QWidget* parent) : QWizardPage(parent) {
    setTitle(tr("Game Boots"));

    header = new QLabel(tr("Does the game boot?"));

    game_boot = new QButtonGroup(this);
    game_boot_yes = new QRadioButton(tr("Yes   The game starts to output video or audio"));
    game_boot_no =
        new QRadioButton(tr("No    The game doesn't get past the \"Launching...\" screen"));
    game_boot->addButton(game_boot_yes, 0);
    game_boot->addButton(game_boot_no, 1);

    layout = new QVBoxLayout();
    layout->addWidget(header);
    layout->insertStretch(1, 20);
    layout->addWidget(game_boot_yes);
    layout->addWidget(game_boot_no);
    setLayout(layout);

    registerField(boot_yes, game_boot_yes);
    registerField(boot_no, game_boot_no);

    connect(game_boot, &QButtonGroup::idClicked, this, &QWizardPage::completeChanged);
}

bool BootPage::isComplete() const {
    return game_boot->checkedId() != -1;
}

GamePlayPage::GamePlayPage(QWidget* parent) : QWizardPage(parent) {
    setTitle(tr("Game Starts"));

    header = new QLabel(tr("Does the game reach gameplay?"));

    game_play = new QButtonGroup(this);
    game_play_yes =
        new QRadioButton(tr("Yes   The game gets past the intro/menu and into gameplay"));
    game_play_no =
        new QRadioButton(tr("No    The game crashes or freezes while loading or using the menu"));
    game_play->addButton(game_play_yes, 0);
    game_play->addButton(game_play_no, 1);

    layout = new QVBoxLayout();
    layout->addWidget(header);
    layout->insertStretch(1, 20);
    layout->addWidget(game_play_yes);
    layout->addWidget(game_play_no);
    setLayout(layout);

    registerField(play_yes, game_play_yes);
    registerField(play_no, game_play_no);

    connect(game_play, &QButtonGroup::idClicked, this, &QWizardPage::completeChanged);
}

bool GamePlayPage::isComplete() const {
    return game_play->checkedId() != -1;
}

FreezePage::FreezePage(QWidget* parent) : QWizardPage(parent) {
    setTitle(tr("Game Stability"));

    header = new QLabel(
        tr("Does the game work without crashing, freezing or locking up during gameplay?"));
    header->setWordWrap(true);

    freeze = new QButtonGroup(this);
    freeze_yes = new QRadioButton(tr("Yes   The game works without crashes"));
    freeze_no = new QRadioButton(tr("No    The game crashes or freezes during gameplay"));
    freeze->addButton(freeze_yes, 0);
    freeze->addButton(freeze_no, 1);

    layout = new QVBoxLayout();
    layout->addWidget(header);
    layout->insertStretch(1, 20);
    layout->addWidget(freeze_yes);
    layout->addWidget(freeze_no);
    setLayout(layout);

    registerField(freeze_yes_, freeze_yes);
    registerField(freeze_no_, freeze_no);

    connect(freeze, &QButtonGroup::idClicked, this, &QWizardPage::completeChanged);
}

bool FreezePage::isComplete() const {
    return freeze->checkedId() != -1;
}

CompletePage::CompletePage(QWidget* parent) : QWizardPage(parent) {
    setTitle(tr("Game Completion"));

    header = new QLabel(tr("Is the game completely playable from start to finish?"));

    complete = new QButtonGroup(this);
    complete_yes = new QRadioButton(tr("Yes   The game can be finished without any workarounds"));
    complete_no = new QRadioButton(tr("No    The game can't progress past a certain area"));
    complete->addButton(complete_yes, 0);
    complete->addButton(complete_no, 1);

    layout = new QVBoxLayout();
    layout->addWidget(header);
    layout->insertStretch(1, 20);
    layout->addWidget(complete_yes);
    layout->addWidget(complete_no);
    setLayout(layout);

    registerField(complete_yes_, complete_yes);
    registerField(complete_no_, complete_no);

    connect(complete, &QButtonGroup::idClicked, this, &QWizardPage::completeChanged);
}

bool CompletePage::isComplete() const {
    return complete->checkedId() != -1;
}

GraphicalPage::GraphicalPage(QWidget* parent) : QWizardPage(parent) {
    setTitle(tr("Graphical Problems"));

    header = new QLabel(tr("Does the game have any graphical glitches?"));

    graphical = new QButtonGroup(this);
    graphical_major = new QRadioButton(tr("Major   The game has major graphical errors"));
    graphical_minor = new QRadioButton(tr("Minor   The game has minor graphical errors"));
    graphical_none =
        new QRadioButton(tr("None     Everything is rendered as it looks on the Nintendo Switch"));
    graphical->addButton(graphical_major, 0);
    graphical->addButton(graphical_minor, 1);
    graphical->addButton(graphical_none, 2);

    layout = new QVBoxLayout();
    layout->addWidget(header);
    layout->insertStretch(1, 20);
    layout->addWidget(graphical_major);
    layout->addWidget(graphical_minor);
    layout->addWidget(graphical_none);
    setLayout(layout);

    registerField(graphical_major_, graphical_major);
    registerField(graphical_minor_, graphical_minor);
    registerField(graphical_none_, graphical_none);

    connect(graphical, &QButtonGroup::idClicked, this, &QWizardPage::completeChanged);
}

bool GraphicalPage::isComplete() const {
    return graphical->checkedId() != -1;
}

AudioPage::AudioPage(QWidget* parent) : QWizardPage(parent) {
    setTitle(tr("Audio Problems"));

    header = new QLabel(tr("Does the game have any audio glitches / missing effects?"));

    audio = new QButtonGroup(this);
    audio_major = new QRadioButton(tr("Major   The game has major audio errors"));
    audio_minor = new QRadioButton(tr("Minor   The game has minor audio errors"));
    audio_none = new QRadioButton(tr("None     Audio is played perfectly"));
    audio->addButton(audio_major, 0);
    audio->addButton(audio_minor, 1);
    audio->addButton(audio_none, 2);

    layout = new QVBoxLayout();
    layout->addWidget(header);
    layout->insertStretch(1, 20);
    layout->addWidget(audio_major);
    layout->addWidget(audio_minor);
    layout->addWidget(audio_none);
    setLayout(layout);

    registerField(audio_major_, audio_major);
    registerField(audio_minor_, audio_minor);
    registerField(audio_none_, audio_none);

    connect(audio, &QButtonGroup::idClicked, this, &QWizardPage::completeChanged);
}

bool AudioPage::isComplete() const {
    return audio->checkedId() != -1;
}

CommentPage::CommentPage(QWidget* parent) : QWizardPage(parent) {
    setTitle(tr("Comments"));

    header = new QLabel(tr("Do you want to add a comment?"));

    comment = new QButtonGroup(this);
    comment_yes = new QRadioButton(tr("Yes   Include comment"));
    comment_no = new QRadioButton(tr("No    Don't include comment"));
    comment->addButton(comment_yes, 0);
    comment->addButton(comment_no, 1);

    editor = new QTextEdit();
    editor->setUndoRedoEnabled(true);
    editor->setWordWrapMode(QTextOption::WordWrap);

    layout = new QVBoxLayout();
    layout->addWidget(header);
    layout->insertStretch(1, 20);
    layout->addWidget(comment_yes);
    layout->addWidget(comment_no);
    layout->addWidget(editor);
    setLayout(layout);

    registerField(comment_text, editor, "plainText");
    registerField(comment_use, comment_yes);

    connect(comment, &QButtonGroup::idClicked, this, &QWizardPage::completeChanged);
    connect(editor, &QTextEdit::textChanged, this, &QWizardPage::completeChanged);
}

bool CommentPage::isComplete() const {
    if (comment->checkedId() == 1) {
        return true;
    }
    if (editor->document()->isEmpty()) {
        return false;
    }
    return true;
}
// clang-format off
ReviewPage::ReviewPage(std::string* host_, std::string* cookie_, std::string* csrf_,
                       std::string yuzu_version_, std::string game_version_,
                       std::string program_id_, std::string cpu_model_,
                       std::string cpu_brand_string_, std::string ram_, std::string swap_,
                       std::string gpu_vendor_, std::string gpu_model_, std::string gpu_version_,
                       std::string os_, Settings::Values& settings_values_,
                       QNetworkAccessManager& network_manager_, QWidget* parent)
    : QWizardPage(parent), yuzu_version{std::move(yuzu_version_)},
      game_version{std::move(game_version_)}, program_id{std::move(program_id_)},
      cpu_model{std::move(cpu_model_)}, cpu_brand_string{std::move(cpu_brand_string_)},
      ram{std::move(ram_)}, swap{std::move(swap_)}, gpu_vendor{std::move(gpu_vendor_)},
      gpu_model{std::move(gpu_model_)}, gpu_version{std::move(gpu_version_)}, os{std::move(os_)},
      host{host_}, cookie{cookie_}, csrf{csrf_}, network_manager{network_manager_},
      settings_values{settings_values_} {
    // clang-format on
    setTitle(tr("Review Time"));

    review = new QLabel(tr("Review of report will go here"));
    review->setWordWrap(true);
    label = new QLabel(tr("Review Name"));
    label_edit = new QLineEdit();
    label_edit->setMaxLength(32);
    comment = new QLabel(tr("Review details:"));
    comment->setVisible(false);
    comment_view = new QLabel();
    comment_view->setVisible(false);
    rating = new QLabel(tr("Playability:"));
    rating_view = new QLabel();
    report_sent = new QRadioButton(this);
    report_sent->setVisible(false);

    report_send = new QPushButton(tr("Send Report"));
    report_send->setVisible(false);
    report_send->setDisabled(true);

    layout = new QVBoxLayout();
    layout->addWidget(label);
    layout->addWidget(label_edit);
    layout->addWidget(comment);
    layout->addWidget(comment_view);
    layout->addWidget(rating);
    layout->addWidget(rating_view);
    layout->addWidget(review);
    layout->addWidget(report_send);
    setLayout(layout);

    connect(label_edit, &QLineEdit::textChanged, this, &ReviewPage::EnableSend);
    connect(report_sent, &QRadioButton::toggled, this, &QWizardPage::completeChanged);
    connect(report_send, &QPushButton::pressed, this, &ReviewPage::Send);
}

bool ReviewPage::isComplete() const {
    return report_sent->isChecked();
}

void ReviewPage::initializePage() {
    if (!cookie->empty() && csrf->empty()) {
        CSRF();
    }

    CompatibilityStatus compatibility = CalculateCompatibility();
    LOG_INFO(Frontend, "Compatibility Rating: {}", compatibility);

    if (field(comment_use).toBool()) {
        comment->setVisible(true);
        comment_view->setVisible(true);
        comment_view->setText(field(comment_text).toString());
    } else {
        comment->setVisible(false);
        comment_view->setVisible(false);
    }

    switch (compatibility) {
    case CompatibilityStatus::Perfect:
        rating_view->setText(tr("Perfect"));
        break;
    case CompatibilityStatus::IntroMenu:
        rating_view->setText(tr("Intro/Menu"));
        break;
    case CompatibilityStatus::Ingame:
        rating_view->setText(tr("In Game"));
        break;
    case CompatibilityStatus::Playable:
        rating_view->setText(tr("Playable"));
        break;
    case CompatibilityStatus::WontBoot:
        rating_view->setText(tr("Won't Boot"));
        break;
    }

    json settings_details;
    for (auto& [category, settings] : settings_values.linkage.by_category) {
        const char* tmp = Settings::TranslateCategory(category);
        for (const auto& setting : settings) {
            if (setting->Id() == settings_values.yuzu_token.Id()) {
                // Hide the token secret, for security reasons.
                continue;
            }
            if (setting->Id() == settings_values.current_user.Id()) {
                // Hide the current user JSON, for security reasons.
                continue;
            }
            if (setting->Id() == settings_values.report_user.Id()) {
                // Hide the report user JSON, for security reasons.
                continue;
            }
            if (setting->Id() == settings_values.yuzu_cookie.Id()) {
                // Hide the report user cookie, for security reasons.
                continue;
            }
            if (setting->Id() == settings_values.yuzu_username.Id()) {
                // Hide the report user name, because the server already has that.
                continue;
            }

            settings_details[tmp][setting->GetLabel()] = setting->Canonicalize();
        }
    }

    json device_details;
    device_details["Settings"] = settings_details;
    device_details["Host"] = {{"OS", os},
                              {"CPU Brand", cpu_brand_string},
                              {"CPU Model", cpu_model},
                              {"RAM", ram},
                              {"Swap", swap},
                              {"GPU Vendor", gpu_vendor},
                              {"GPU Model", gpu_model},
                              {"GPU Driver", gpu_version}};

    report = {{"game", {{"target_id", program_id}}},
              {"version", {{"value", game_version}}},
              {"playability", {{"target_id", compatibility}}},
              {"description", {{"value", field(comment_text).toString().toStdString()}}},
              {"device_details", {{"value", device_details.dump()}}},
              {"yuzu_version", {{"value", yuzu_version}}}};

    review->setText(QString::fromStdString(report.dump()));
}

void ReviewPage::OnCSRF(QNetworkReply* reply) {
    network_manager.disconnect(SIGNAL(finished(QNetworkReply*)));
    if (reply->error() != QNetworkReply::NoError) {
        emit NetworkError(reply->errorString());
        LOG_WARNING(Frontend, "Failed to get CSRF-Token: {}:{}", reply->errorString().toStdString(),
                    reply->readAll().constData());
        return;
    }

    emit SetCSRF(reply->readAll());
    report_send->setVisible(true);
}

void ReviewPage::CSRF() {
    if (!csrf->empty()) {
        return;
    }
    auto url = *host + TOKEN;
    connect(&network_manager, &QNetworkAccessManager::finished,
            [this](QNetworkReply* reply) { OnCSRF(reply); });

    QNetworkRequest request;
    request.setTransferTimeout(3000);
    request.setAttribute(QNetworkRequest::AutoDeleteReplyOnFinishAttribute, true);
    request.setRawHeader(COOKIE.c_str(), cookie->c_str());
    request.setUrl(QUrl(QString::fromStdString(url)));
    network_manager.get(request);
}

void ReviewPage::EnableSend(const QString& text) {
    if (text.isEmpty()) {
        report_send->setDisabled(true);
        return;
    }
    if (report_send->isEnabled()) {
        return;
    }
    report_send->setDisabled(false);
}

void ReviewPage::OnSend(QNetworkReply* reply) {
    network_manager.disconnect(SIGNAL(finished(QNetworkReply*)));
    if (reply->error() != QNetworkReply::NoError) {
        emit NetworkError(reply->errorString());
        LOG_WARNING(Frontend, "Report Failed: {}:{}", reply->errorString().toStdString(),
                    reply->readAll().constData());
        return;
    }

    emit ReportMade(reply->readAll());
    report_sent->setChecked(true);
    report_send->setDisabled(true);
    report_send->setText(tr("Report Sent"));
}

void ReviewPage::Send() {
    emit ClearErrors();
    auto url = *host + ADD;

    report["label"] = {{"value", label_edit->text().toStdString()}};

    connect(&network_manager, &QNetworkAccessManager::finished,
            [this](QNetworkReply* reply) { OnSend(reply); });

    QNetworkRequest request;
    request.setTransferTimeout(3000);
    request.setAttribute(QNetworkRequest::AutoDeleteReplyOnFinishAttribute, true);
    request.setUrl(QUrl(QString::fromStdString(url)));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QVariant::fromValue<QString>(content_json));
    request.setRawHeader(COOKIE.c_str(), cookie->c_str());
    request.setRawHeader(CSRF_TOKEN.c_str(), csrf->c_str());
    request.setTransferTimeout(3000);
    request.setAttribute(QNetworkRequest::AutoDeleteReplyOnFinishAttribute, true);
    network_manager.post(request, report.dump().c_str());
}

CompatibilityStatus ReviewPage::CalculateCompatibility() const {
    if (field(boot_no).toBool()) {
        return CompatibilityStatus::WontBoot;
    }

    if (field(play_no).toBool()) {
        return CompatibilityStatus::IntroMenu;
    }

    if (field(freeze_no_).toBool() || field(complete_no_).toBool()) {
        return CompatibilityStatus::Ingame;
    }

    if (field(graphical_major_).toBool() || field(audio_major_).toBool()) {
        return CompatibilityStatus::Ingame;
    }

    if (field(graphical_minor_).toBool() || field(audio_minor_).toBool()) {
        return CompatibilityStatus::Playable;
    }

    return CompatibilityStatus::Perfect;
}

ScreenshotsPage::ScreenshotsPage(std::string* host_, std::string* cookie_, std::string* csrf_,
                                 std::string* report_, std::string screenshot_path_,
                                 QNetworkAccessManager& network_manager_, QWidget* parent)
    : QWizardPage(parent), screenshot_path{std::move(screenshot_path_)}, host{host_},
      cookie{cookie_}, csrf{csrf_}, report{report_}, network_manager{network_manager_} {
    setTitle(tr("Screenshots"));

    header = new QLabel(tr("Do you want to add screenshots?"));

    screenshots = new QButtonGroup(this);
    screenshots_yes = new QRadioButton(tr("Yes   Include Screenshots"));
    screenshots_no = new QRadioButton(tr("No    Don't Include Screenshots"));
    screenshots->addButton(screenshots_yes, 0);
    screenshots->addButton(screenshots_no, 1);

    edit_sent = new QRadioButton(this);
    edit_sent->setVisible(false);
    upload = new QPushButton();
    upload->setText(tr("Upload"));
    upload->setVisible(false);
    del = new QPushButton();
    del->setText(tr("Remove"));
    del->setVisible(false);

    file_list = new QListWidget();
    file_picker = new QFileDialog(this);
    file_picker->setFileMode(QFileDialog::ExistingFiles);

    auto h_lay = new QHBoxLayout();
    h_lay->addWidget(upload);
    h_lay->addWidget(del);

    info = new QLabel();
    info->setText(tr("* Click Yes to add more images * 3 max *"));
    info->setVisible(false);

    status = new QLabel();
    status->setWordWrap(true);
    status->setVisible(false);

    upload_progress = new QProgressBar();
    upload_progress->setMinimum(8);
    upload_progress->setVisible(false);

    layout = new QVBoxLayout();
    layout->addWidget(header);
    layout->addWidget(screenshots_yes);
    layout->addWidget(screenshots_no);
    layout->addWidget(file_list);
    layout->addItem(h_lay);
    layout->addWidget(status);
    layout->addWidget(upload_progress);
    layout->addWidget(info);
    setLayout(layout);

    registerField(screenshot_yes_, screenshots_yes);
    registerField(screenshot_no_, screenshots_no);

    connect(upload, &QPushButton::clicked, this, &ScreenshotsPage::UploadFiles);
    connect(del, &QPushButton::clicked, this, &ScreenshotsPage::RemoveFiles);
    connect(file_list, &QListWidget::itemClicked, this, &ScreenshotsPage::PickItems);
    connect(screenshots_yes, &QRadioButton::pressed, this, &ScreenshotsPage::PickFiles);
    connect(screenshots, &QButtonGroup::idClicked, this, &QWizardPage::completeChanged);
    connect(edit_sent, &QRadioButton::toggled, this, &QWizardPage::completeChanged);
}

void ScreenshotsPage::initializePage() {
    json_report = json::parse(report->c_str());
}

void ScreenshotsPage::OnUploadURL(QNetworkReply* reply) {
    network_manager.disconnect(SIGNAL(finished(QNetworkReply*)));
    if (reply->error() != QNetworkReply::NoError) {
        emit NetworkError(reply->errorString());
        LOG_WARNING(Frontend, "Upload DCU Failed: {}:{}", reply->errorString().toStdString(),
                    reply->readAll().constData());
        status->setText(tr("Failed to get upload URL!"));
        return;
    }

    dcus[imgs] = json::parse(reply->readAll().constData());
}

void ScreenshotsPage::GetUploadURL() {
    auto url = *host + DCU;
    connect(&network_manager, &QNetworkAccessManager::finished,
            [this](QNetworkReply* reply_) { OnUploadURL(reply_); });

    QNetworkRequest request;
    request.setTransferTimeout(3000);
    request.setAttribute(QNetworkRequest::AutoDeleteReplyOnFinishAttribute, true);
    request.setUrl(QUrl(QString::fromStdString(url)));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QVariant::fromValue<QString>(content_json));
    request.setRawHeader(COOKIE.c_str(), cookie->c_str());
    auto id = json_report["id"][0]["value"];
    request.setRawHeader(REPORT_ID.c_str(), id.dump().c_str());
    request.setTransferTimeout(3000);
    request.setAttribute(QNetworkRequest::AutoDeleteReplyOnFinishAttribute, true);
    QNetworkReply* reply = network_manager.post(request, "");

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
}

void ScreenshotsPage::OnUpload(QNetworkReply* reply) {
    network_manager.disconnect(SIGNAL(finished(QNetworkReply*)));
    upload_progress->setVisible(false);
    upload_progress->reset();
    if (reply->error() != QNetworkReply::NoError) {
        emit NetworkError(reply->errorString());
        LOG_WARNING(Frontend, "Screenshots Upload Failed: {}:{}",
                    reply->errorString().toStdString(), reply->readAll().constData());
        status->setText(tr("Uploading Failed"));
        failed++;
        return;
    }

    auto res = reply->readAll();
    dcus[imgs] = json::parse(res.constData());
    status->setText(tr("Image Uploaded"));
    uploaded++;
    LOG_INFO(Frontend, "Image Uploaded: {}", res.constData());
}

void ScreenshotsPage::UploadProgress(qint64 bytesSent, qint64 bytesTotal) {
    upload_progress->setValue(bytesSent);
}

void ScreenshotsPage::UploadFiles() {
    emit ClearErrors();

    upload->setText(tr("Uploading..."));
    upload->setDisabled(true);

    status->setText(tr("Uploading Images"));
    status->setVisible(true);

    uploaded = 0;
    failed = 0;

    for (int i = 0; i < file_list->count(); i++) {
        QListWidgetItem* item = file_list->item(i);
        if (item->checkState() != Qt::Checked) {
            continue;
        }
        if (!dcus[i].empty() && dcus[i]["success"] && !dcus[i]["result"].contains("uploadURL")) {
            continue;
        }
        imgs = i;
        if (dcus[i].empty() || !dcus[i]["success"]) {
            GetUploadURL();
        }
        if (dcus[i].empty() || !dcus[i]["success"]) {
            continue;
        }

        QHttpMultiPart* multi_part = new QHttpMultiPart(QHttpMultiPart::FormDataType);

        QString file_path = item->data(Qt::UserRole).toString();
        QFile* file = new QFile(file_path);
        if (!file->open(QIODevice::ReadOnly)) {
            LOG_ERROR(Frontend, "FAILED TO OPEN FILE: {}", file_path.toStdString());
            status->setText(tr("FAILED TO OPEN FILE: %1").arg(file_path));
            failed++;
            break;
        }
        file->setParent(multi_part);

        QHttpPart image_part;
        std::string name = file->fileName().toStdString();
        std::string tmp = "form-data; name=\"file\"; filename=\"" + name + "\"";
        image_part.setHeader(QNetworkRequest::ContentDispositionHeader,
                             QVariant::fromValue<QByteArray>(tmp.c_str()));
        image_part.setBodyDevice(file);
        multi_part->append(image_part);

        upload_progress->setMaximum(file->size());

        QNetworkRequest request;
        auto url = dcus[i]["result"]["uploadURL"];
        request.setUrl(QUrl(QString::fromStdString(url)));
        request.setTransferTimeout(180000); // 3 minutes
        request.setAttribute(QNetworkRequest::AutoDeleteReplyOnFinishAttribute, true);

        status->setText(tr("Uploading: %1").arg(file_path));
        connect(&network_manager, &QNetworkAccessManager::finished,
                [this](QNetworkReply* reply_) { OnUpload(reply_); });

        QNetworkReply* reply = network_manager.post(request, multi_part);
        connect(reply, &QNetworkReply::uploadProgress, this, &ScreenshotsPage::UploadProgress);
        upload_progress->setVisible(true);
        multi_part->setParent(reply);

        QEventLoop reply_loop;
        connect(reply, &QNetworkReply::finished, &reply_loop, &QEventLoop::quit);
        reply_loop.exec();
    }

    if (failed > 0) {
        upload->setText(tr("Try Again"));
        upload->setDisabled(false);
        status->setText(tr("Uploaded: %1, Failed: %2 \n Try Again").arg(uploaded).arg(failed));
        return;
    }

    if (uploaded > 0) {
        EditReport();
    }
}

void ScreenshotsPage::OnEdit(QNetworkReply* reply) {
    network_manager.disconnect(SIGNAL(finished(QNetworkReply*)));
    if (reply->error() != QNetworkReply::NoError) {
        emit NetworkError(reply->errorString());
        status->setText(tr("Report Update Failed"));
        LOG_WARNING(Frontend, "Report Update Failed: {}:{}", reply->errorString().toStdString(),
                    reply->readAll().constData());
        return;
    }

    auto res = reply->readAll();
    emit ReportChange(res);
    status->setText(tr("Report Updated"));
    edit_sent->setChecked(true);
    LOG_INFO(Frontend, "Report Update: {}", res.constData());
}

void ScreenshotsPage::EditReport() {
    emit ClearErrors();
    status->setText(tr("Updating your report"));
    auto url = *host + EDIT + json_report["id"][0]["value"].dump().c_str() + "?_format=json";

    json update;
    auto json_objects = json::array();
    for (size_t i = 0; i < dcus.size(); i++) {
        if (!dcus[i].empty() && dcus[i]["success"]) {
            json_objects.push_back(json::object({{"value", dcus[i]["result"]["variants"][0]}}));
        }
    }

    update["screenshots"] = json_objects;

    connect(&network_manager, &QNetworkAccessManager::finished,
            [this](QNetworkReply* reply) { OnEdit(reply); });

    QNetworkRequest request;
    request.setTransferTimeout(3000);
    request.setAttribute(QNetworkRequest::AutoDeleteReplyOnFinishAttribute, true);
    request.setUrl(QUrl(QString::fromStdString(url)));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QVariant::fromValue<QString>(content_json));
    request.setRawHeader(COOKIE.c_str(), cookie->c_str());
    request.setRawHeader(CSRF_TOKEN.c_str(), csrf->c_str());
    request.setTransferTimeout(3000);
    request.setAttribute(QNetworkRequest::AutoDeleteReplyOnFinishAttribute, true);
    network_manager.sendCustomRequest(request, "PATCH", update.dump().c_str());
}

void ScreenshotsPage::RemoveFiles() {
    auto item = file_list->currentItem();
    delete item;
    del->setVisible(false);
    Checked();
}

void ScreenshotsPage::PickFiles() {
    screenshots_yes->setChecked(true);
    const auto files = file_picker->getOpenFileNames(
        parentWidget(), tr("Screenshots"), QString::fromStdString(screenshot_path), file_filter);
    for (const QString& file : files) {
        if (file_list->count() > 2) {
            break;
        }
        QListWidgetItem* item = new QListWidgetItem(QFileInfo(file).fileName(), file_list);
        item->setData(Qt::UserRole, file);
        item->setFlags(item->flags() | Qt::ItemNeverHasChildren);
        item->setCheckState(Qt::Unchecked);
    }
    info->setVisible(true);
}

void ScreenshotsPage::PickItems(QListWidgetItem* item) {
    if (item->checkState() == Qt::Checked) {
        upload->setVisible(true);
    } else {
        Checked();
    }
    del->setVisible(true);
}

bool ScreenshotsPage::isComplete() const {
    if (screenshots->checkedId() == 1 || edit_sent->isChecked()) {
        return true;
    }
    return false;
}

ThankYouPage::ThankYouPage(QWidget* parent) : QWizardPage(parent) {
    setTitle(tr("Thank you for your submission!"));
}

CompatDB::CompatDB(std::string yuzu_version_, std::string game_version_, std::string program_id_,
                   std::string cpu_model_, std::string cpu_brand_string_, std::string ram_,
                   std::string swap_, std::string gpu_vendor_, std::string gpu_model_,
                   std::string gpu_version_, std::string os_, Settings::Values& settings_values_,
                   std::string screenshot_path_, QWidget* parent)
    : QWizard(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint),
      host{std::move(settings_values_.report_api_url.GetValue())},
      username{std::move(settings_values_.yuzu_username.GetValue())},
      token{std::move(settings_values_.yuzu_token.GetValue())},
      cookie{std::move(settings_values_.yuzu_cookie.GetValue())},
      current_user{std::move(settings_values_.report_user.GetValue())} {
    network_manager = new QNetworkAccessManager(this);
    network_manager->setStrictTransportSecurityEnabled(true);

    setMinimumSize(500, 410);
    setWindowTitle(tr("Report Compatibility"));
    setOptions(QWizard::DisabledBackButtonOnLastPage | QWizard::HelpButtonOnRight |
               QWizard::NoBackButtonOnStartPage);

    setPage(Page_Intro,
            new IntroPage(&host, &username, &token, &cookie, &current_user, *network_manager));
    setPage(Page_GameBoot, new BootPage());
    setPage(Page_GamePlay, new GamePlayPage());
    setPage(Page_Freeze, new FreezePage());
    setPage(Page_Complete, new CompletePage());
    setPage(Page_Graphical, new GraphicalPage());
    setPage(Page_Audio, new AudioPage());
    setPage(Page_Comment, new CommentPage());
    setPage(Page_Review,
            new ReviewPage(&host, &cookie, &csrf_token, yuzu_version_, game_version_, program_id_,
                           cpu_model_, cpu_brand_string_, ram_, swap_, gpu_vendor_, gpu_model_,
                           gpu_version_, os_, settings_values_, *network_manager));
    setPage(Page_Screenshots, new ScreenshotsPage(&host, &cookie, &csrf_token, &report,
                                                  screenshot_path_, *network_manager));
    setPage(Page_ThankYou, new ThankYouPage());

    network_errors = new QLabel();
    network_errors->setWordWrap(true);
    setSideWidget(network_errors);

    connect(qobject_cast<IntroPage*>(page(Page_Intro)), &IntroPage::UserChange, this,
            &CompatDB::OnUserChange);
    connect(qobject_cast<IntroPage*>(page(Page_Intro)), &IntroPage::NetworkError, this,
            &CompatDB::NetworkError);
    connect(qobject_cast<IntroPage*>(page(Page_Intro)), &IntroPage::ClearErrors, this,
            &CompatDB::ClearErrors);
    connect(qobject_cast<IntroPage*>(page(Page_Intro)), &IntroPage::LoggedOut, this,
            &CompatDB::Logout);
    connect(qobject_cast<ReviewPage*>(page(Page_Review)), &ReviewPage::NetworkError, this,
            &CompatDB::NetworkError);
    connect(qobject_cast<ReviewPage*>(page(Page_Review)), &ReviewPage::ClearErrors, this,
            &CompatDB::ClearErrors);
    connect(qobject_cast<ReviewPage*>(page(Page_Review)), &ReviewPage::ReportMade, this,
            &CompatDB::ReportMade);
    connect(qobject_cast<ReviewPage*>(page(Page_Review)), &ReviewPage::SetCSRF, this,
            &CompatDB::SetCSRFToken);
    connect(qobject_cast<ScreenshotsPage*>(page(Page_Screenshots)), &ScreenshotsPage::NetworkError,
            this, &CompatDB::NetworkError);
    connect(qobject_cast<ScreenshotsPage*>(page(Page_Screenshots)), &ScreenshotsPage::ClearErrors,
            this, &CompatDB::ClearErrors);
    connect(qobject_cast<ScreenshotsPage*>(page(Page_Screenshots)), &ScreenshotsPage::ReportChange,
            this, &CompatDB::ReportMade);
}

int CompatDB::nextId() const {
    switch (currentId()) {
    case Page_Intro:
        return Page_GameBoot;
    case Page_GameBoot:
        if (field(boot_no).toBool()) {
            return Page_Comment;
        }
        return Page_GamePlay;
    case Page_GamePlay:
        if (field(play_no).toBool()) {
            return Page_Comment;
        }
        return Page_Freeze;
    case Page_Freeze:
        if (field(freeze_no_).toBool()) {
            return Page_Comment;
        }
        return Page_Complete;
    case Page_Complete:
        if (field(complete_no_).toBool()) {
            return Page_Comment;
        }
        return Page_Graphical;
    case Page_Graphical:
        return Page_Audio;
    case Page_Audio:
        return Page_Comment;
    case Page_Comment:
        return Page_Review;
    case Page_Review:
        return Page_Screenshots;
    case Page_Screenshots:
        return Page_ThankYou;
    case Page_ThankYou:
        return -1;
    default:
        LOG_ERROR(Frontend, "Unexpected page: {}", currentId());
        return Page_Intro;
    }
}

void CompatDB::OnUserChange() {
    emit UserChange(QString::fromLatin1(cookie.c_str()), QString::fromLatin1(current_user.c_str()));
}

void CompatDB::NetworkError(QString error) {
    network_errors->setText(error);
}

void CompatDB::ClearErrors() {
    network_errors->clear();
}

void CompatDB::Logout() {
    done(QDialog::Rejected);
}

void CompatDB::ReportMade(QByteArray report_) {
    report = report_.toStdString();
}

void CompatDB::SetCSRFToken(QByteArray csrf) {
    csrf_token = csrf.toStdString();
}
