#pragma once

#include <QString>

namespace passcave {

    typedef struct {
        QString type;
        QString username;
        QString secret;
        QString issuer;
        QString algorithm;
        int digits;
        int period;
        int counter;
    } Info2FA;

    Info2FA parse2FAUri(QString const& uri);

    QString generateOTP(Info2FA const& info);
    int generateTOTP(Info2FA const& info);
    QString generateTOTPForSteamGuard(Info2FA const& info, bool secretIsBase64Encoded = false);
    int generateHOTP(Info2FA const& info);
}
