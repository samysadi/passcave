#include "gcry.h"
#include "config.h"
#include "utils.h"

#include <unordered_map>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdlib>

namespace passcave {
std::string GCRYPT_LAST_ERROR_STRING;
}

using namespace passcave;

std::string passcave::gcrypt_getLastErrorString() {
	return passcave::GCRYPT_LAST_ERROR_STRING;
}

int passcave::gcrypt_init() {
	GCRYPT_LAST_ERROR_STRING = "";
	static bool initialized = false;
	if (initialized)
		return GCRYPT_INIT_SUCCESS;

	/* Version check should be the very first call because it
	makes sure that important subsystems are intialized. */
	if (!gcry_check_version(GCRYPT_MINIMUM_VERSION)) {
		GCRYPT_LAST_ERROR_STRING = "gcrypt: library version too old (at least version " + std::string(GCRYPT_MINIMUM_VERSION) + " is required).";
		return GCRYPT_INIT_ERROR_VERSION;
	}

	gcry_error_t err = 0;

	/* We don't want to see any warnings, e.g. because we have not yet
	parsed program options which might be used to suppress such
	warnings. */
	err = gcry_control (GCRYCTL_SUSPEND_SECMEM_WARN);

	/* ... If required, other initialization goes here.  Note that the
	process might still be running with increased privileges and that
	the secure memory has not been intialized.  */

	/* Allocate a pool of 16k secure memory.  This make the secure memory
	available and also drops privileges where needed.  */
	err |= gcry_control (GCRYCTL_INIT_SECMEM, 16384, 0);

	/* It is now okay to let Libgcrypt complain when there was/is
	a problem with the secure memory. */
	err |= gcry_control (GCRYCTL_RESUME_SECMEM_WARN);

	/* ... If required, other initialization goes here.  */

	/* Tell Libgcrypt that initialization has completed. */
	err |= gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);

	if (err) {
		GCRYPT_LAST_ERROR_STRING = "gcrypt: failed initialization.";
		return GCRYPT_INIT_ERROR_OTHER;
	}

	initialized = true;

	return GCRYPT_INIT_SUCCESS;
}

std::unordered_map<uint32_t, gcry_md_algos>* mdMap = NULL;
std::unordered_map<uint32_t, gcry_md_algos> const& gcrypt_mdMap() {
	if (mdMap != NULL)
		return *mdMap;
	mdMap = new std::unordered_map<uint32_t, gcry_md_algos>();
	std::unordered_map<uint32_t, gcry_md_algos>& m = *mdMap;
	m[0]	= GCRY_MD_NONE;
	m[1]	= GCRY_MD_MD5;
	m[2]	= GCRY_MD_SHA1;
	m[3]	= GCRY_MD_RMD160;
	m[5]	= GCRY_MD_MD2;
	m[6]	= GCRY_MD_TIGER;
	m[7]	= GCRY_MD_HAVAL;
	m[8]	= GCRY_MD_SHA256;
	m[9]	= GCRY_MD_SHA384;
	m[10]	= GCRY_MD_SHA512;
	m[11]	= GCRY_MD_SHA224;
	m[301]	= GCRY_MD_MD4;
	m[302]	= GCRY_MD_CRC32;
	m[303]	= GCRY_MD_CRC32_RFC1510;
	m[304]	= GCRY_MD_CRC24_RFC2440;
	m[305]	= GCRY_MD_WHIRLPOOL;
	m[306]	= GCRY_MD_TIGER1;
	m[307]	= GCRY_MD_TIGER2;
	m[308]	= GCRY_MD_GOSTR3411_94;
	m[309]	= GCRY_MD_STRIBOG256;
	m[310]	= GCRY_MD_STRIBOG512;
	m[311]	= GCRY_MD_GOSTR3411_CP;
	m[312]	= GCRY_MD_SHA3_224;
	m[313]	= GCRY_MD_SHA3_256;
	m[314]	= GCRY_MD_SHA3_384;
	m[315]	= GCRY_MD_SHA3_512;
	m[316]	= GCRY_MD_SHAKE128;
	m[317]	= GCRY_MD_SHAKE256;
    m[318]  = GCRY_MD_BLAKE2B_512;
    m[319]  = GCRY_MD_BLAKE2B_384;
    m[320]  = GCRY_MD_BLAKE2B_256;
    m[321]  = GCRY_MD_BLAKE2B_160;
    m[322]  = GCRY_MD_BLAKE2S_256;
    m[323]  = GCRY_MD_BLAKE2S_224;
    m[324]  = GCRY_MD_BLAKE2S_160;
    m[325]  = GCRY_MD_BLAKE2S_128;
    m[326]  = GCRY_MD_SM3;
    m[327]  = GCRY_MD_SHA512_256;
    m[328]  = GCRY_MD_SHA512_224;
	return *mdMap;
}

std::unordered_map<gcry_md_algos, uint32_t>* mdMapRev = NULL;
std::unordered_map<gcry_md_algos, uint32_t> const& gcrypt_mdMapRev() {
	if (mdMapRev != NULL)
		return *mdMapRev;
	mdMapRev = new std::unordered_map<gcry_md_algos, uint32_t>();
	std::unordered_map<gcry_md_algos, uint32_t>& m = *mdMapRev;
	auto m0 = gcrypt_mdMap();
	for (auto const& v: m0)
		m[v.second] = v.first;
	return *mdMapRev;
}

std::vector<gcry_md_algos> passcave::gcrypt_getMdAlgos(bool sorted) {
	std::vector<gcry_md_algos> r;
	for (auto v: gcrypt_mdMap()) {
		if (v.second == GCRY_MD_NONE)
			continue;
		r.push_back(v.second);
	}

	if (sorted) {
		std::sort(r.begin(), r.end(), [](gcry_md_algos const& a, gcry_md_algos const& b) {
			return simple_compareStrings(toString(a), toString(b)) < 0;
		});
	}

	return r;
}

gcry_md_algos passcave::gcrypt_mapIntToMd(uint32_t v) {
	auto const& r = gcrypt_mdMap().find(v);
	if (r == gcrypt_mdMap().end())
		return GCRY_MD_NONE;
	return r->second;
}

uint32_t passcave::gcrypt_mapMdToInt(gcry_md_algos v) {
	auto const& r = gcrypt_mdMapRev().find(v);
	if (r == gcrypt_mdMapRev().end())
		return 0;
	return r->second;
}

std::string passcave::toString(gcry_md_algos v) {
	if (v == GCRY_MD_NONE || gcrypt_init() != GCRYPT_INIT_SUCCESS)
		return std::string();
	std::string s = std::string(gcry_md_algo_name(v));
	if (s.empty() || s.compare("?") == 0)
		return std::string("MD_") + std::to_string(static_cast<int>(v));
	return s;
}

std::unordered_map<uint32_t, gcry_cipher_algos>* cipherMap = NULL;
std::unordered_map<uint32_t, gcry_cipher_algos> const& gcrypt_cipherMap() {
	if (cipherMap != NULL)
		return *cipherMap;
	cipherMap = new std::unordered_map<uint32_t, gcry_cipher_algos>();
	std::unordered_map<uint32_t, gcry_cipher_algos>& m = *cipherMap;
	m[0]	= GCRY_CIPHER_NONE;
	m[1]	= GCRY_CIPHER_IDEA;
	m[2]	= GCRY_CIPHER_3DES;
	m[3]	= GCRY_CIPHER_CAST5;
	m[4]	= GCRY_CIPHER_BLOWFISH;
	m[5]	= GCRY_CIPHER_SAFER_SK128;
	m[6]	= GCRY_CIPHER_DES_SK;
	m[7]	= GCRY_CIPHER_AES;
	m[8]	= GCRY_CIPHER_AES192;
	m[9]	= GCRY_CIPHER_AES256;
	m[10]	= GCRY_CIPHER_TWOFISH;
	m[301]	= GCRY_CIPHER_ARCFOUR;
	m[302]	= GCRY_CIPHER_DES;
	m[303]	= GCRY_CIPHER_TWOFISH128;
	m[304]	= GCRY_CIPHER_SERPENT128;
	m[305]	= GCRY_CIPHER_SERPENT192;
	m[306]	= GCRY_CIPHER_SERPENT256;
	m[307]	= GCRY_CIPHER_RFC2268_40;
	m[308]	= GCRY_CIPHER_RFC2268_128;
	m[309]	= GCRY_CIPHER_SEED;
	m[310]	= GCRY_CIPHER_CAMELLIA128;
	m[311]	= GCRY_CIPHER_CAMELLIA192;
	m[312]	= GCRY_CIPHER_CAMELLIA256;
	m[313]	= GCRY_CIPHER_SALSA20;
	m[314]	= GCRY_CIPHER_SALSA20R12;
	m[315]	= GCRY_CIPHER_GOST28147;
	m[316]	= GCRY_CIPHER_CHACHA20;
    m[317]  = GCRY_CIPHER_GOST28147_MESH;
    m[318]  = GCRY_CIPHER_SM4;
	return *cipherMap;
}

std::unordered_map<gcry_cipher_algos, uint32_t>* cipherMapRev = NULL;
std::unordered_map<gcry_cipher_algos, uint32_t> const& gcrypt_cipherMapRev() {
	if (cipherMapRev != NULL)
		return *cipherMapRev;
	cipherMapRev = new std::unordered_map<gcry_cipher_algos, uint32_t>();
	std::unordered_map<gcry_cipher_algos, uint32_t>& m = *cipherMapRev;
	auto m0 = gcrypt_cipherMap();
	for (auto const& v: m0)
		m[v.second] = v.first;
	return *cipherMapRev;
}

std::vector<gcry_cipher_algos> passcave::gcrypt_getCipherAlgos(bool sorted) {
	std::vector<gcry_cipher_algos> r;
	for (auto v: gcrypt_cipherMap()) {
		if (v.second == GCRY_CIPHER_NONE)
			continue;
		r.push_back(v.second);
	}

	if (sorted) {
		std::sort(r.begin(), r.end(), [](gcry_cipher_algos const& a, gcry_cipher_algos const& b) {
			return simple_compareStrings(toString(a), toString(b)) < 0;
		});
	}

	return r;
}

gcry_cipher_algos passcave::gcrypt_mapIntToCipherAlgo(uint32_t v) {
	auto const& r = gcrypt_cipherMap().find(v);
	if (r == gcrypt_cipherMap().end())
		return GCRY_CIPHER_NONE;
	return r->second;
}

uint32_t passcave::gcrypt_mapCipherAlgoToInt(gcry_cipher_algos v) {
	auto const& r = gcrypt_cipherMapRev().find(v);
	if (r == gcrypt_cipherMapRev().end())
		return 0;
	return r->second;
}

std::string passcave::toString(gcry_cipher_algos v) {
	if (v == GCRY_CIPHER_NONE || gcrypt_init() != GCRYPT_INIT_SUCCESS)
		return std::string();
	std::string s = std::string(gcry_cipher_algo_name(v));
	if (s.empty() || s.compare("?") == 0)
		return std::string("Cipher_") + std::to_string(static_cast<int>(v));
	return s;
}

std::unordered_map<uint32_t, gcry_cipher_modes>* cipherModeMap = NULL;
std::unordered_map<gcry_cipher_modes, std::string>* cipherModeMapString = NULL;
std::unordered_map<uint32_t, gcry_cipher_modes> const& gcrypt_cipherModeMap() {
	if (cipherModeMap != NULL)
		return *cipherModeMap;
	cipherModeMap = new std::unordered_map<uint32_t, gcry_cipher_modes>();
	std::unordered_map<uint32_t, gcry_cipher_modes>& m = *cipherModeMap;
	m[0]	= GCRY_CIPHER_MODE_NONE;
	m[1]	= GCRY_CIPHER_MODE_ECB;
	m[2]	= GCRY_CIPHER_MODE_CFB;
	m[3]	= GCRY_CIPHER_MODE_CBC;
	m[4]	= GCRY_CIPHER_MODE_STREAM;
	m[5]	= GCRY_CIPHER_MODE_OFB;
	m[6]	= GCRY_CIPHER_MODE_CTR;
	m[7]	= GCRY_CIPHER_MODE_AESWRAP;
	m[8]	= GCRY_CIPHER_MODE_CCM;
	m[9]	= GCRY_CIPHER_MODE_GCM;
	m[10]	= GCRY_CIPHER_MODE_POLY1305;
	m[11]	= GCRY_CIPHER_MODE_OCB;
	m[12]	= GCRY_CIPHER_MODE_CFB8;
    m[13]   = GCRY_CIPHER_MODE_XTS;
    m[14]   = GCRY_CIPHER_MODE_EAX;
    m[15]   = GCRY_CIPHER_MODE_SIV;
    m[16]   = GCRY_CIPHER_MODE_GCM_SIV;


	cipherModeMapString = new std::unordered_map<gcry_cipher_modes, std::string>();
	std::unordered_map<gcry_cipher_modes, std::string>& s = *cipherModeMapString;
	s[GCRY_CIPHER_MODE_NONE]		= "NONE";
	s[GCRY_CIPHER_MODE_ECB]			= "ECB";
	s[GCRY_CIPHER_MODE_CFB]			= "CFB";
	s[GCRY_CIPHER_MODE_CBC]			= "CBC";
	s[GCRY_CIPHER_MODE_STREAM]		= "STREAM";
	s[GCRY_CIPHER_MODE_OFB]			= "OFB";
	s[GCRY_CIPHER_MODE_CTR]			= "CTR";
	s[GCRY_CIPHER_MODE_AESWRAP]		= "AESWRAP";
	s[GCRY_CIPHER_MODE_CCM]			= "CCM";
	s[GCRY_CIPHER_MODE_GCM]			= "GCM";
	s[GCRY_CIPHER_MODE_POLY1305]	= "POLY1305";
	s[GCRY_CIPHER_MODE_OCB]			= "OCB";
	s[GCRY_CIPHER_MODE_CFB8]		= "CFB8";
    s[GCRY_CIPHER_MODE_XTS]         = "XTS";
    s[GCRY_CIPHER_MODE_EAX]         = "EAX";
    s[GCRY_CIPHER_MODE_SIV]         = "SIV";
    s[GCRY_CIPHER_MODE_GCM_SIV]     = "GCM_SIV";

	return *cipherModeMap;
}

std::unordered_map<gcry_cipher_modes, uint32_t>* cipherModeMapRev = NULL;
std::unordered_map<gcry_cipher_modes, uint32_t> const& gcrypt_cipherModeMapRev() {
	if (cipherModeMapRev != NULL)
		return *cipherModeMapRev;
	cipherModeMapRev = new std::unordered_map<gcry_cipher_modes, uint32_t>();
	std::unordered_map<gcry_cipher_modes, uint32_t>& m = *cipherModeMapRev;
	auto m0 = gcrypt_cipherModeMap();
	for (auto const& v: m0)
		m[v.second] = v.first;
	return *cipherModeMapRev;
}

std::vector<gcry_cipher_modes> passcave::gcrypt_getCipherModes(bool sorted) {
	std::vector<gcry_cipher_modes> r;
	for (auto v: gcrypt_cipherModeMap()) {
		if (v.second == GCRY_CIPHER_MODE_NONE)
			continue;
		r.push_back(v.second);
	}

	if (sorted) {
		std::sort(r.begin(), r.end(), [](gcry_cipher_modes const& a, gcry_cipher_modes const& b) {
			return simple_compareStrings(toString(a), toString(b)) < 0;
		});
	}
	return r;
}

gcry_cipher_modes passcave::gcrypt_mapIntToCipherMode(uint32_t v) {
	auto const& r = gcrypt_cipherModeMap().find(v);
	if (r == gcrypt_cipherModeMap().end())
		return GCRY_CIPHER_MODE_NONE;
	return r->second;
}

uint32_t passcave::gcrypt_mapCipherModeToInt(gcry_cipher_modes v) {
	auto const& r = gcrypt_cipherModeMapRev().find(v);
	if (r == gcrypt_cipherModeMapRev().end())
		return 0;
	return r->second;
}

std::string passcave::toString(gcry_cipher_modes v) {
	if (v == GCRY_CIPHER_MODE_NONE || gcrypt_init() != GCRYPT_INIT_SUCCESS)
		return std::string();
	gcrypt_cipherModeMap();
	auto const& r = cipherModeMapString->find(v);
	if (r == cipherModeMapString->end() || r->second.compare("?") == 0)
		return std::string("Mode_") + std::to_string(static_cast<int>(v));
	return r->second;
}

std::vector<char> passcave::genRandom(int size) {
	if (gcrypt_init() != GCRYPT_INIT_SUCCESS) {
		std::vector<char> v;
		while (size-- > 0)
			v.push_back(static_cast<char>(rand()));
		return v;
	}

	char t[size];
	gcry_randomize(t, size, GCRY_VERY_STRONG_RANDOM);
	std::vector<char> r;
	r.insert(r.begin(), t, t + size);
	secureErase(t, size);
	return r;
}

int passcave::genRandomInt(int max) {
	if (gcrypt_init() != GCRYPT_INIT_SUCCESS)
		return rand() % max;

	int i = 0;
	gcry_randomize(&i, sizeof(i), GCRY_VERY_STRONG_RANDOM);
	if (i < 0)
		i = -i;
	return (i % max);
}

int passcave::genRandomMdIterations() {
	return (1 << 19) + genRandomInt(1024);
}

std::vector<char> passcave::gcrypt_computeHash(gcry_md_algos gcry_md_algo, void const* data, int size, int iterations) {
	GCRYPT_LAST_ERROR_STRING = "";
	if (gcrypt_init() != GCRYPT_INIT_SUCCESS)
		return std::vector<char>();

	int hash_length = gcry_md_get_algo_dlen(gcry_md_algo);
	unsigned char hash[hash_length];

	gcry_md_hash_buffer(gcry_md_algo, hash, data, size);
	iterations--;
	if (iterations > 0) {
		int newDataLength = hash_length + size;
		unsigned char newData[newDataLength];
		std::copy(reinterpret_cast<unsigned char const*>(data), reinterpret_cast<unsigned char const*>(data) + size, &newData[hash_length]);
		while (iterations-- != 0) {
			std::copy(hash, hash + hash_length, newData);
			gcry_md_hash_buffer(gcry_md_algo, hash, newData, newDataLength);
		}
		secureErase(newData, newDataLength);
	}

	std::vector<char> v(hash, hash + hash_length);
	secureErase(hash, hash_length);
	return v;
}

int passcave::gcrypt_mdSize(gcry_md_algos gcry_algo) {
	if (gcry_algo == GCRY_MD_NONE || gcrypt_init() != GCRYPT_INIT_SUCCESS)
		return 0;
	return gcry_md_get_algo_dlen(gcry_algo);
}

int passcave::gcrypt_algo_keyLength(gcry_cipher_algos gcry_algo) {
	if (gcry_algo == GCRY_CIPHER_NONE || gcrypt_init() != GCRYPT_INIT_SUCCESS)
		return 0;
	return gcry_cipher_get_algo_keylen(gcry_algo);
}

inline char* getInitialIV(void const* key, int key_size, void const* arr, int arr_size, int initIVSize) {
	char* IV = new char[initIVSize];
	unsigned char const* pKey = reinterpret_cast<unsigned char const*>(key);
	if (key == NULL)
		key_size = 0;
	unsigned char const* pArr = reinterpret_cast<unsigned char const*>(arr);
	if (arr == NULL)
		arr_size = 0;
	int j = 0;
	int a = 0;
	int k = 0;
	for (int i = 0; i<initIVSize; i++) {
		unsigned char x = GCRY_INIT_IV[j];
		j = (j + 1) % GCRY_INIT_IV_SIZE;

		if (key_size != 0) {
			x = x ^ *(pKey + k);
			k = (k + 1) % key_size;
		}

		if (arr_size != 0) {
			x = x ^ *(pArr + a);
			k = (k + 1) % arr_size;
		}

		*(IV+i) = x;
	}
	return IV;
}

inline std::vector<char> gcrypt_decryptOrEncryptData(bool const encrypting, int gcry_cipher_algo, int cipher_mode,
												   void const* key0, int key_size0, void const* data, int size,
												   void* initIV, int initIVSize) {
	GCRYPT_LAST_ERROR_STRING = "";
	if (gcrypt_init() != GCRYPT_INIT_SUCCESS)
		return std::vector<char>();
	if (size <= 0)
		return std::vector<char>();

	char* key;
	int key_size;

	size_t max_key_size = gcry_cipher_get_algo_keylen(gcry_cipher_algo);
	if (key_size0 > max_key_size) {
		//shorten key
		key_size = max_key_size;
		key = new char[key_size];
		std::copy(reinterpret_cast<char const*>(key0), reinterpret_cast<char const*>(key0) + key_size, key);
		int k = 0;
		for (int k0 = key_size; k0<key_size0; k0++) {
			key[k] = key[k] ^ *(reinterpret_cast<char const*>(key0) + k0);
			k = (k + 1) % key_size;
		}
	} else {
		key_size = key_size0;
		key = const_cast<char*>(reinterpret_cast<char const*>(key0)); //don't panic! we wont modify key0
	}

	size_t blkLength = gcry_cipher_get_algo_blklen(gcry_cipher_algo);

	gcry_cipher_hd_t hd;
	gcry_error_t gcryError;

	gcryError = gcry_cipher_open(&hd, gcry_cipher_algo, cipher_mode, 0);
	if (gcryError) {
		GCRYPT_LAST_ERROR_STRING = "gcrypt: gcry_cipher_open failed: " + std::string(gcry_strsource(gcryError)) + "/" + std::string(gcry_strerror(gcryError)) + ".";
		return std::vector<char>();
	}

	// Set key
	gcryError = gcry_cipher_setkey(hd, key, key_size);
	if (gcryError) {
		GCRYPT_LAST_ERROR_STRING = "gcrypt: gcry_cipher_setkey failed: " + std::string(gcry_strsource(gcryError)) + "/" + std::string(gcry_strerror(gcryError)) + ".";
		gcry_cipher_close(hd);
		return std::vector<char>();
	}

	// Set initial IV
	{
		char* IV = getInitialIV(key, key_size, initIV, initIVSize, blkLength);
		initIVSize = blkLength;

		gcryError = gcry_cipher_setiv(hd, IV, initIVSize);
		if (gcryError) {
			GCRYPT_LAST_ERROR_STRING = "gcrypt: gcry_cipher_setiv failed: " + std::string(gcry_strsource(gcryError)) + "/" + std::string(gcry_strerror(gcryError)) + ".";
			gcry_cipher_close(hd);
			return std::vector<char>();
		}

		secureErase(IV, initIVSize);
		delete[] IV;
	}

	// Clear key if it's ours
	if (key != key0) {
		secureErase(key, key_size);
		delete[] key;
	}

	uint32_t padSize = 0;
	int roundedSize;

	if (encrypting) {
		// Write padding + padding size
		roundedSize = (size + sizeof(uint32_t)) % blkLength;
		if (roundedSize == 0)
			roundedSize = size + sizeof(uint32_t);
		else {
			padSize = blkLength - roundedSize;
			roundedSize = size + sizeof(uint32_t) + padSize;
		}
	} else
		roundedSize = size;

	char out[roundedSize];
	std::copy(reinterpret_cast<char const*>(data), reinterpret_cast<char const*>(data) + size, out);
	if (encrypting) {
		*reinterpret_cast<uint32_t*>(out + roundedSize - sizeof(uint32_t)) = padSize;
		if (padSize > 0)
			gcry_randomize(out + size, padSize, GCRY_VERY_STRONG_RANDOM);
	}

	// encrypting / decrypting
	int const roundedSize0 = roundedSize;
	if (encrypting)
		gcryError = gcry_cipher_encrypt(hd, out, roundedSize, NULL, 0);
	else {
		gcryError = gcry_cipher_decrypt(hd, out, roundedSize, NULL, 0);

		//remove padding
		padSize = *reinterpret_cast<uint32_t*>(out + size - sizeof(uint32_t));
		if (padSize >= blkLength) {
			//corrupt data / bad key
			secureErase(out, roundedSize0);
			gcry_cipher_close(hd);
			return std::vector<char>();
		} else {
			roundedSize = roundedSize - padSize - sizeof(uint32_t);
		}
	}
	if (gcryError) {
		GCRYPT_LAST_ERROR_STRING = "gcrypt: gcry_cipher_" + std::string(encrypting ? "encrypt" : "decrypt") + " failed: " + std::string(gcry_strsource(gcryError)) + "/" + std::string(gcry_strerror(gcryError)) + ".";
		secureErase(out, roundedSize0);
		gcry_cipher_close(hd);
		return std::vector<char>();
	}

	gcry_cipher_close(hd);

	std::vector<char> r(out, out + roundedSize);
	secureErase(out, roundedSize0);

	return r;
}

std::vector<char> passcave::gcrypt_encryptData(int gcry_cipher_algo, int cipher_mode,
										void const* key, int key_size, void const* data, int size,
										void* initIV, int initIVSize) {
	return gcrypt_decryptOrEncryptData(true, gcry_cipher_algo, cipher_mode, key, key_size, data, size, initIV, initIVSize);
}

std::vector<char> passcave::gcrypt_decryptData(int gcry_cipher_algo, int cipher_mode,
										void const* key, int key_size, void const* data, int size,
										void* initIV, int initIVSize) {
	return gcrypt_decryptOrEncryptData(false, gcry_cipher_algo, cipher_mode, key, key_size, data, size, initIV, initIVSize);
}

//generates a unique signature
inline uint32_t genSignature() {
	return GCRY_PASSCAVE_SIGNATURE;
}

inline bool checkSignature(uint32_t const& a) {
	return a == GCRY_PASSCAVE_SIGNATURE;
}

// reads signature + fileInfos and stops before IV
inline bool writeFileInfo(gcrypt_FileInfo const& fileInfo, std::ofstream& outFile) {
	uint32_t a;

	//write signature
	a = genSignature();
	if (!outFile.write(reinterpret_cast<char*>(&a), sizeof(a)))
		return false;

	//write meta data
	gcry_randomize(&a, sizeof(a), GCRY_VERY_STRONG_RANDOM);
	if (fileInfo.usingCompression && a == 0)
		a = 1;
	if (!fileInfo.usingCompression && a != 0)
		a = 0;
	if (!outFile.write(reinterpret_cast<char*>(&a), sizeof(a)))
		return false;

	a = gcrypt_mapMdToInt(fileInfo.gcry_md_algo);
	if (!outFile.write(reinterpret_cast<char*>(&a), sizeof(a)))
		return false;

	a = gcrypt_mapCipherAlgoToInt(fileInfo.gcry_cipher_algo);
	if (!outFile.write(reinterpret_cast<char*>(&a), sizeof(a)))
		return false;

	a = gcrypt_mapCipherModeToInt(fileInfo.gcry_cipher_mode);
	if (!outFile.write(reinterpret_cast<char*>(&a), sizeof(a)))
		return false;

	a = fileInfo.gcry_md_iterations;
	if (!outFile.write(reinterpret_cast<char*>(&a), sizeof(a)))
		return false;

	a = fileInfo.gcry_md_iterations_auto ? 1 : 0;
	if (!outFile.write(reinterpret_cast<char*>(&a), sizeof(a)))
		return false;

	//write version
	a = fileInfo.passcave_version_min;
	if (!outFile.write(reinterpret_cast<char*>(&a), sizeof(a)))
		return false;

	a = fileInfo.passcave_version_maj;
	if (!outFile.write(reinterpret_cast<char*>(&a), sizeof(a)))
		return false;

	return true;
}

// reads signature + fileInfos and stops before IV
inline bool readFileInfo(gcrypt_FileInfo& fileInfo, std::ifstream& inFile) {
	uint32_t a = 0;
	//read signature
	if (!inFile.read(reinterpret_cast<char*>(&a), sizeof(a)))
		return false;
	if (!checkSignature(a))
		return false;

	//read meta data
	if (!inFile.read(reinterpret_cast<char*>(&a), sizeof(a)))
		return false;
	fileInfo.usingCompression = a != 0;

	if (!inFile.read(reinterpret_cast<char*>(&a), sizeof(a)))
		return false;
	fileInfo.gcry_md_algo = gcrypt_mapIntToMd(a);
	if (fileInfo.gcry_md_algo == GCRY_MD_NONE)
		return false;

	if (!inFile.read(reinterpret_cast<char*>(&a), sizeof(a)))
		return false;
	fileInfo.gcry_cipher_algo = gcrypt_mapIntToCipherAlgo(a);
	if (fileInfo.gcry_cipher_algo == GCRY_CIPHER_NONE)
		return false;

	if (!inFile.read(reinterpret_cast<char*>(&a), sizeof(a)))
		return false;
	fileInfo.gcry_cipher_mode = gcrypt_mapIntToCipherMode(a);
	if (fileInfo.gcry_cipher_mode == GCRY_CIPHER_MODE_NONE)
		return false;

	if (!inFile.read(reinterpret_cast<char*>(&a), sizeof(a)))
		return false;
	fileInfo.gcry_md_iterations = static_cast<uint32_t>(a);

	if (!inFile.read(reinterpret_cast<char*>(&a), sizeof(a)))
		return false;
	fileInfo.gcry_md_iterations_auto = a != 0;

	//read version
	if (!inFile.read(reinterpret_cast<char*>(&a), sizeof(a)))
		return false;
	fileInfo.passcave_version_min = static_cast<uint32_t>(a);

	if (!inFile.read(reinterpret_cast<char*>(&a), sizeof(a)))
		return false;
	fileInfo.passcave_version_maj = static_cast<uint32_t>(a);

	return true;
}

bool passcave::gcrypt_encryptToFile(std::string filename, std::string const& clearKey,
									gcrypt_FileInfo* fileInfo,
									void const* data, int size) {
	GCRYPT_LAST_ERROR_STRING = "";
	if (gcrypt_init() != GCRYPT_INIT_SUCCESS)
		return false;
	if (size == 0)
		return true;
	if (fileInfo == NULL)
		return false;
	if (!fileInfo->isValid())
		return false;

	//hash key
	if (fileInfo->gcry_md_iterations_auto)
		fileInfo->gcry_md_iterations = genRandomMdIterations();
	std::vector<char> vKey = gcrypt_computeHash(fileInfo->gcry_md_algo, clearKey.c_str(), clearKey.size(), fileInfo->gcry_md_iterations);

	//generate IV
	int IVSize = gcry_cipher_get_algo_blklen(fileInfo->gcry_md_algo);
	char IV[IVSize];
	gcry_randomize(IV, IVSize, GCRY_VERY_STRONG_RANDOM);

	std::vector<char> v;
	{
		#ifdef passcave_NO_ZLIB_COMPRESS
		void const* pData = data;
		int dataSize = size;
		#else
		//compress data
		std::vector<char> compressedData = bzipCompress(data, size);
		char const* pData = compressedData.data();
		int dataSize = compressedData.size();
		#endif

		v = passcave::gcrypt_encryptData(fileInfo->gcry_cipher_algo, fileInfo->gcry_cipher_mode, vKey.data(), vKey.size(), pData, dataSize, IV, IVSize);

		#ifndef passcave_NO_ZLIB_COMPRESS
		secureErase(compressedData);
		#endif
	}

	secureErase(vKey);
	if (v.size() == 0) {
		#ifdef passcave_PARANOID_MEMORY_MANAGEMENT
		secureErase(IV, IVSize);
		#endif
		return false;
	}

	bool r = true;
	std::ofstream outFile(filename, std::ios::out | std::ios::binary | std::ios::trunc);
	if (outFile) {
#ifdef passcave_NO_ZLIB_COMPRESS
		fileInfo->usingCompression = false;
#else
		fileInfo->usingCompression = true;
#endif

		r = writeFileInfo(*fileInfo, outFile);

		//write IV
		uint32_t a = static_cast<uint32_t>(IVSize);
		r = r && outFile.write(reinterpret_cast<char*>(&a), sizeof(a));
		r = r && outFile.write(IV, IVSize);

		// write actual data
		if (!r || !outFile.write(v.data(), v.size())) {
			GCRYPT_LAST_ERROR_STRING = "Error when writing data to output file \"" + filename + "\"";
			r = false;
		}
		outFile.close();
	} else {
		#ifdef passcave_PARANOID_MEMORY_MANAGEMENT
		secureErase(v);// v is encrypted though
		secureErase(IV, IVSize);
		#endif
		GCRYPT_LAST_ERROR_STRING = "Cannot open output file \"" + filename + "\"";
		return false;
	}

	#ifdef passcave_PARANOID_MEMORY_MANAGEMENT
	secureErase(v);// v is encrypted though
	secureErase(IV, IVSize);
	#endif

	return r;
}

std::vector<char> passcave::gcrypt_decryptFromFile(std::string filename, std::string const& clearKey, gcrypt_FileInfo* fileInfoPtr) {
	GCRYPT_LAST_ERROR_STRING = "";
	if (gcrypt_init() != GCRYPT_INIT_SUCCESS)
		return std::vector<char>();

	gcrypt_FileInfo fileInfo;
	std::ifstream inFile(filename, std::ios::in | std::ios::binary | std::ios::ate);
	if (inFile) {
		std::ifstream::pos_type size = inFile.tellg();
		inFile.seekg(0, std::ios::beg);

		bool r = readFileInfo(fileInfo, inFile);
		if (fileInfoPtr != NULL)
			*fileInfoPtr = fileInfo;
		if (!fileInfo.isValid())
			return std::vector<char>();

		//read IV
		uint32_t a = 0;
		r = r && inFile.read(reinterpret_cast<char*>(&a), sizeof(a));
		int IVSize = static_cast<int>(a);
		if (IVSize < 0)
			IVSize = 0;
		char IV[IVSize];
		if (IVSize != 0)
			r = r && inFile.read(IV, IVSize);

		//read actual data
		size = size - inFile.tellg();
		std::vector<char> inData(size);
		if (!r || !inFile.read(inData.data(), size)) {
			GCRYPT_LAST_ERROR_STRING = "Error when reading data from input file \"" + filename + "\"";
			#ifdef passcave_PARANOID_MEMORY_MANAGEMENT
			secureErase(inData);
			#endif
			fileInfo.secureErase();
			inFile.close();
			return std::vector<char>();
		}
		inFile.close();

		std::vector<char> vKey = gcrypt_computeHash(fileInfo.gcry_md_algo, clearKey.c_str(), clearKey.size(), fileInfo.gcry_md_iterations);

		std::vector<char> v = passcave::gcrypt_decryptData(fileInfo.gcry_cipher_algo, fileInfo.gcry_cipher_mode, vKey.data(), vKey.size(), inData.data(), size, IV, IVSize);
		secureErase(vKey);
		#ifdef passcave_PARANOID_MEMORY_MANAGEMENT
		secureErase(IV, IVSize);
		secureErase(inData);
		#endif

		if (v.empty() || !fileInfo.usingCompression) {
			fileInfo.secureErase();
			return v;
		} else {
			fileInfo.secureErase();
			//uncompress data
			std::vector<char> v2 = bzipUncompress(v.data(), v.size());
			secureErase(v);
			return v2;
		}
	} else {
		GCRYPT_LAST_ERROR_STRING = "Cannot open input file \"" + filename + "\"";
		return std::vector<char>();
	}
}

gcrypt_FileInfo passcave::gcrypt_getFileInfo(std::string filename) {
	GCRYPT_LAST_ERROR_STRING = "";

	gcrypt_FileInfo fileInfo;
	std::ifstream inFile(filename, std::ios::in | std::ios::binary | std::ios::ate);
	if (inFile) {
		std::ifstream::pos_type size = inFile.tellg();
		inFile.seekg(0, std::ios::beg);

		bool r = readFileInfo(fileInfo, inFile);
		if (!r) {
			GCRYPT_LAST_ERROR_STRING = "Error when reading data from input file \"" + filename + "\"";
			fileInfo.secureErase();
			inFile.close();
			return fileInfo;
		}
		inFile.close();

		return fileInfo;
	} else {
		GCRYPT_LAST_ERROR_STRING = "Cannot open input file \"" + filename + "\"";
		fileInfo.secureErase();
		return fileInfo;
	}
}
