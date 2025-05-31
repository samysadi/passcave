#include "utils2fa.h"
#include "gcry.h"

#include <QUrl>
#include <QUrlQuery>
#include <QString>

#include <gcrypt.h>

#include <algorithm>
#include <cmath>
#include <ctime>
#include <cstdint>
#include <string>


using namespace passcave;

Info2FA passcave::parse2FAUri(const QString &uri) {
    QUrl url(uri);
    QUrlQuery query(url);

    Info2FA info;
    info.type = url.host();
    QString path = url.path();
    path = QUrl::fromPercentEncoding(path.toUtf8());
    if (path.startsWith('/')) {
        path.remove(0, 1); // Remove the first character
    }
    info.username = path;
    info.secret = query.queryItemValue("secret");
    info.issuer = query.queryItemValue("issuer");
    info.algorithm = query.queryItemValue("algorithm");
    info.digits = query.hasQueryItem("digits") ? query.queryItemValue("digits").toInt() : 6;
    info.period = query.hasQueryItem("period") ? query.queryItemValue("period").toInt() : 30;
    info.counter = query.hasQueryItem("counter") ? query.queryItemValue("counter").toInt() : 0;

    return info;
}

int decode_base32_char(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= '2' && c <= '7') return c - '2' + 26;
    return -1;
}

std::string base32_decode(std::string const& encoded) {
    std::string decoded;
    int buffer = 0;
    int bitsLeft = 0;
    for (char c : encoded) {
        if (c == ' ' || c == '=') {
            continue;
        }
        int val = decode_base32_char(c);
        if (val == -1) break;
        buffer <<= 5;
        buffer |= val;
        bitsLeft += 5;
        if (bitsLeft >= 8) {
            decoded.push_back((char) ((buffer >> (bitsLeft - 8)) & 0xFF));
            bitsLeft -= 8;
        }
    }
    return decoded;
}

std::string base64_decode(const std::string &in) {
    std::string out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i; 

    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

std::string hmac_sha1(const std::string &key, const std::string &data) {
    unsigned char hmac[gcry_md_get_algo_dlen(GCRY_MD_SHA1)];
    gcry_error_t gcryError;
    gcry_md_hd_t handle;
    gcryError = gcry_md_open(&handle, GCRY_MD_SHA1, GCRY_MD_FLAG_HMAC);
    if (gcryError) {
        return std::string();
    }
    gcryError = gcry_md_setkey(handle, key.data(), key.size());
    if (gcryError) {
        gcry_md_close(handle);
        return std::string();
    }
    gcry_md_write(handle, data.data(), data.size());
    gcry_md_final(handle);
    memcpy(hmac, gcry_md_read(handle, GCRY_MD_SHA1), gcry_md_get_algo_dlen(GCRY_MD_SHA1));
    gcry_md_close(handle);
    return std::string(reinterpret_cast<char*>(hmac), sizeof(hmac));
}

int getMdFromStr(QString const& algo) {
    auto a = algo.toUpper().trimmed();
    for (auto md: gcrypt_getMdAlgos()) {
        auto b = QString(gcry_md_algo_name(md)).toUpper();
        if (a.compare(b) == 0) {
            return md;
        }
    }
    return GCRY_MD_NONE;
}

uint64_t getTimeStep(Info2FA const& info) {
    return std::time(nullptr) / info.period;
}

QString passcave::generateOTP(Info2FA const& info) {
    auto t = info.type.trimmed().toLower();
    if (t.compare("hotp") == 0) {
        return QString::number(generateHOTP(info));
    } else {
        if (info.issuer.trimmed().toLower().compare("steam") == 0) {
            return generateTOTPForSteamGuard(info);
        } else {
            return QString::number(generateTOTP(info));
        }
    }
}

int passcave::generateTOTP(Info2FA const& info) {
    // Decode the base32-encoded secret
    std::string key = base32_decode(info.secret.toStdString());

    // Get the current time and calculate the time step
    uint64_t timeStep = getTimeStep(info);

    // Convert time step to big-endian byte array
    unsigned char message[8];
    for (int i = 7; i >= 0; --i) {
        message[i] = timeStep & 0xFF;
        timeStep >>= 8;
    }

    // Initialize gcrypt
    gcry_error_t gcryError;
    gcry_md_hd_t md;
    gcryError = gcry_md_open(&md, getMdFromStr(info.algorithm), GCRY_MD_FLAG_HMAC);
    if (gcryError) {
        return 0;
    }

    // Set the HMAC key
    gcryError = gcry_md_setkey(md, key.data(), key.size());
    if (gcryError) {
        gcry_md_close(md);
        return 0;
    }

    // Calculate HMAC hash
    gcry_md_write(md, message, 8);
    gcry_md_final(md);
    unsigned char* hash = gcry_md_read(md, 0);

    // Dynamic truncation to get the OTP
    int offset = hash[19] & 0xf;
    int binary = ((hash[offset] & 0x7f) << 24) |
                 ((hash[offset + 1] & 0xff) << 16) |
                 ((hash[offset + 2] & 0xff) << 8) |
                 (hash[offset + 3] & 0xff);

    int otp = binary % static_cast<int>(std::pow(10.f, info.digits));

    // Close the hash handle
    gcry_md_close(md);

    return otp;
}

const std::string STEAM_GUARD_CHARSET = "23456789BCDFGHJKMNPQRTVWXY";
QString passcave::generateTOTPForSteamGuard(Info2FA const& info, bool secretIsBase64Encoded) {
    std::string key = secretIsBase64Encoded ? base64_decode(info.secret.toStdString()) : base32_decode(info.secret.toStdString());
    uint64_t timeStep = getTimeStep(info);
    std::string packed_time = std::string(reinterpret_cast<const char*>(&timeStep), 8);
    std::reverse(packed_time.begin(), packed_time.end());
    std::string hmac = hmac_sha1(key, packed_time);
    if (hmac.empty()) {
        return QString();
    }
    int start = static_cast<unsigned char>(hmac[19]) & 0x0F;
    uint32_t codeint = (static_cast<uint32_t>(static_cast<unsigned char>(hmac[start]) & 0x7f) << 24) |
                       (static_cast<uint32_t>(static_cast<unsigned char>(hmac[start + 1]) << 16)) |
                       (static_cast<uint32_t>(static_cast<unsigned char>(hmac[start + 2]) << 8)) |
                       static_cast<uint32_t>(static_cast<unsigned char>(hmac[start + 3]));

    std::string code;
    for (int i = 0; i < 5; ++i) {
        code += STEAM_GUARD_CHARSET[codeint % STEAM_GUARD_CHARSET.size()];
        codeint /= STEAM_GUARD_CHARSET.size();
    }
    return QString::fromStdString(code);
}

int passcave::generateHOTP(Info2FA const& info) {
    // Decode the base32-encoded secret
    std::string key = base32_decode(info.secret.toStdString());

    // Convert counter to big-endian byte array
    int counter = info.counter;
    unsigned char message[8];
    for (int i = 7; i >= 0; --i) {
        message[i] = counter & 0xFF;
        counter >>= 8;
    }

    // Initialize gcrypt
    gcry_error_t gcryError;
    gcry_md_hd_t md;
    gcryError = gcry_md_open(&md, getMdFromStr(info.algorithm), GCRY_MD_FLAG_HMAC);
    if (gcryError) {
        return 0;
    }

    // Set the HMAC key
    gcryError = gcry_md_setkey(md, key.data(), key.size());
    if (gcryError) {
        gcry_md_close(md);
        return 0;
    }

    // Calculate HMAC hash
    gcry_md_write(md, message, 8);
    gcry_md_final(md);
    unsigned char* hash = gcry_md_read(md, 0);

    // Dynamic truncation to get the OTP
    int offset = hash[19] & 0xf;
    int binary = ((hash[offset] & 0x7f) << 24) |
                 ((hash[offset + 1] & 0xff) << 16) |
                 ((hash[offset + 2] & 0xff) << 8) |
                 (hash[offset + 3] & 0xff);

    int otp = binary % static_cast<int>(std::pow(10, info.digits));

    // Close the hash handle
    gcry_md_close(md);

    return otp;
}

