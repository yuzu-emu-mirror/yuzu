// SPDX-FileCopyrightText: 2023 Yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QButtonGroup>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QVariant>
#include <QWizard>
#include <QWizardPage>

#include <nlohmann/json.hpp>
#include "common/settings.h"
#include "core/telemetry_session.h"

using json = nlohmann::json;

using DirectCreatorUploadResponses = std::array<json, 3>;

enum class CompatibilityStatus {
    Perfect = 7,
    Playable = 8,
    Ingame = 9,
    IntroMenu = 10,
    WontBoot = 11,
};

class IntroPage : public QWizardPage {
    Q_OBJECT

public:
    IntroPage(std::string* host_, std::string* username_, std::string* token_, std::string* cookie_,
              std::string* current_user_, QNetworkAccessManager& network_manager_,
              QWidget* parent = nullptr);
    bool isComplete() const override;
    void OnLogin(QNetworkReply* reply);
    void OnLogout(QNetworkReply* reply);
    void Login();
    void Logout();

signals:
    void UserChange();
    void NetworkError(QString error);
    void ClearErrors();
    void LoggedOut();

private:
    QLabel* cookie_verified;
    QString login;
    QString logout;
    QString ready;
    QPushButton* report_login;
    QPushButton* report_logout;
    QRadioButton* login_yes;
    QLabel* header;
    QVBoxLayout* layout;
    QHBoxLayout* h_layout;

    std::string* host;
    std::string* username;
    std::string* token;
    std::string* cookie;
    std::string* current_user;

    QNetworkAccessManager& network_manager;
};

class BootPage : public QWizardPage {
    Q_OBJECT

public:
    BootPage(QWidget* parent = nullptr);
    bool isComplete() const override;

private:
    QButtonGroup* game_boot;
    QRadioButton* game_boot_yes;
    QRadioButton* game_boot_no;
    QLabel* header;
    QVBoxLayout* layout;
};

class GamePlayPage : public QWizardPage {
    Q_OBJECT

public:
    GamePlayPage(QWidget* parent = nullptr);
    bool isComplete() const override;

private:
    QButtonGroup* game_play;
    QRadioButton* game_play_yes;
    QRadioButton* game_play_no;
    QLabel* header;
    QVBoxLayout* layout;
};

class FreezePage : public QWizardPage {
    Q_OBJECT

public:
    FreezePage(QWidget* parent = nullptr);
    bool isComplete() const override;

private:
    QButtonGroup* freeze;
    QRadioButton* freeze_yes;
    QRadioButton* freeze_no;
    QLabel* header;
    QVBoxLayout* layout;
};

class CompletePage : public QWizardPage {
    Q_OBJECT

public:
    CompletePage(QWidget* parent = nullptr);
    bool isComplete() const override;

private:
    QButtonGroup* complete;
    QRadioButton* complete_yes;
    QRadioButton* complete_no;
    QLabel* header;
    QVBoxLayout* layout;
};

class GraphicalPage : public QWizardPage {
    Q_OBJECT

public:
    GraphicalPage(QWidget* parent = nullptr);
    bool isComplete() const override;

private:
    QButtonGroup* graphical;
    QRadioButton* graphical_major;
    QRadioButton* graphical_minor;
    QRadioButton* graphical_none;
    QLabel* header;
    QVBoxLayout* layout;
};

class AudioPage : public QWizardPage {
    Q_OBJECT

public:
    AudioPage(QWidget* parent = nullptr);
    bool isComplete() const override;

private:
    QButtonGroup* audio;
    QRadioButton* audio_major;
    QRadioButton* audio_minor;
    QRadioButton* audio_none;
    QLabel* header;
    QVBoxLayout* layout;
};

class CommentPage : public QWizardPage {
    Q_OBJECT

public:
    CommentPage(QWidget* parent = nullptr);
    bool isComplete() const override;

private:
    QButtonGroup* comment;
    QRadioButton* comment_yes;
    QRadioButton* comment_no;
    QTextEdit* editor;
    QLabel* header;
    QVBoxLayout* layout;
};

class ReviewPage : public QWizardPage {
    Q_OBJECT

public:
    ReviewPage(std::string* host_, std::string* cookie_, std::string* csrf_,
               std::string yuzu_version_, std::string game_version_, std::string program_id_,
               std::string cpu_model_, std::string cpu_brand_string_, std::string ram_,
               std::string swap_, std::string gpu_vendor_, std::string gpu_model_,
               std::string gpu_version_, std::string os_, Settings::Values& settings_values_,
               QNetworkAccessManager& network_manager_, QWidget* parent = nullptr);
    CompatibilityStatus CalculateCompatibility() const;
    bool isComplete() const override;
    void initializePage() override;

    void CSRF();
    void OnCSRF(QNetworkReply* reply);

    void EnableSend(const QString& text);

    void Send();
    void OnSend(QNetworkReply* reply);

signals:
    void NetworkError(QString error);
    void ClearErrors();
    void ReportMade(QByteArray review);
    void SetCSRF(QByteArray csrf_);

private:
    QRadioButton* report_sent;
    QPushButton* report_send;
    QVBoxLayout* layout;
    QLabel* review;
    QLabel* label;
    QLineEdit* label_edit;
    QLabel* comment;
    QLabel* comment_view;
    QLabel* rating;
    QLabel* rating_view;

    json report;

    std::string yuzu_version;
    std::string game_version;
    std::string program_id;
    std::string cpu_model;
    std::string cpu_brand_string;
    std::string ram;
    std::string swap;
    std::string gpu_vendor;
    std::string gpu_model;
    std::string gpu_version;
    std::string os;

    std::string* host;
    std::string* cookie;
    std::string* csrf;

    QNetworkAccessManager& network_manager;
    Settings::Values& settings_values;
};

class ScreenshotsPage : public QWizardPage {
    Q_OBJECT

public:
    friend class QVariant;
    ScreenshotsPage(std::string* host_, std::string* cookie_, std::string* csrf_,
                    std::string* report_, std::string screenshot_path_,
                    QNetworkAccessManager& network_manager_, QWidget* parent = nullptr);
    bool isComplete() const override;
    void initializePage() override;
    void UploadFiles();
    void RemoveFiles();
    void OnUploadURL(QNetworkReply* reply);
    void GetUploadURL();
    void OnUpload(QNetworkReply* reply);
    void OnEdit(QNetworkReply* reply);
    void EditReport();

    void Checked() {
        bool vis = false;
        for (int i = 0; i < file_list->count(); i++) {
            auto item_ = file_list->item(i);
            if (item_->checkState() == Qt::Checked) {
                vis = true;
            }
        }
        upload->setVisible(vis);
    }
signals:
    void NetworkError(QString error);
    void ClearErrors();
    void ReportChange(QByteArray report_);

public slots:
    void UploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void PickFiles();
    void PickItems(QListWidgetItem* item);

private:
    QRadioButton* edit_sent;
    QButtonGroup* screenshots;
    QRadioButton* screenshots_yes;
    QRadioButton* screenshots_no;
    QPushButton* upload;
    QPushButton* del;
    QLabel* info;
    QLabel* status;
    QLabel* header;
    QVBoxLayout* layout;
    QFileDialog* file_picker;
    QListWidget* file_list;

    std::string screenshot_path;
    std::string* host;
    std::string* cookie;
    std::string* csrf;
    std::string* report;
    json json_report;

    QProgressBar* upload_progress;

    QNetworkAccessManager& network_manager;

    int imgs;
    /**
     * see:
     * https://developers.cloudflare.com/images/cloudflare-images/upload-images/direct-creator-upload/
     */
    DirectCreatorUploadResponses dcus;
    int uploaded;
    int failed;

    const QString file_filter =
        QString::fromLatin1("PNG (*.png);;JPEG (*.jpg *.jpeg);;WEBP (*.webp)");
};

class ThankYouPage : public QWizardPage {
    Q_OBJECT

public:
    ThankYouPage(QWidget* parent = nullptr);
};

class CompatDB : public QWizard {
    Q_OBJECT

public:
    explicit CompatDB(std::string yuzu_verison_, std::string game_version_, std::string program_id_,
                      std::string cpu_model_, std::string cpu_brand_string_, std::string ram_,
                      std::string swap_, std::string gpu_vendor_, std::string gpu_model_,
                      std::string gpu_version_, std::string os_, Settings::Values& settings_values_,
                      std::string screenshot_path_, QWidget* parent = nullptr);
    ~CompatDB() = default;
    int nextId() const override;

    enum {
        Page_Intro,
        Page_GameBoot,
        Page_GamePlay,
        Page_Freeze,
        Page_Complete,
        Page_Graphical,
        Page_Audio,
        Page_Comment,
        Page_Review,
        Page_Screenshots,
        Page_ThankYou
    };

public slots:
    void OnUserChange();
    void NetworkError(QString error);
    void Logout();
    void ClearErrors();
    void SetCSRFToken(QByteArray csrrf_);
    void ReportMade(QByteArray report_);

signals:
    void UserChange(QString cookie, QString user);

private:
    QLabel* network_errors;

    std::string host;
    std::string username;
    std::string token;
    std::string cookie;
    std::string csrf_token;
    std::string current_user;
    std::string report;

    QNetworkAccessManager* network_manager;
};
