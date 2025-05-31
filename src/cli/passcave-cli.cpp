#include "gcry.h"
#include "document.h"
#include "utils.h"

#include <string>
#include <utility>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <iostream>

using namespace passcave;

void printUsage(char** argv) {
	std::cout << "Usage:"  << std::endl;
	std::cout << "\t" << std::string(*argv) << " -i inputFilename"  << std::endl;
	std::cout << "For now only reading operations are supported by the cli version of passcave."  << std::endl;
}

int main(int argc, char** argv) {
	std::unordered_map<std::string, std::string> optionsMap = getCmdOptions(argc, argv);

	struct options_t {
		std::string inputFilename;

	} options;

	for (auto opt: optionsMap) {
		std::string const& n = opt.first;
		if (n == "i" || n == "input")
			options.inputFilename = opt.second;
		else if (n == "h" || n == "help") {
			printUsage(argv);
			return 0;
		} else {
			std::cerr << "Unknown option " << n << ""  << std::endl;
			return 1;
		}
	}

	if (options.inputFilename.empty()) {
		std::cerr << "Please make sure to specify input filename using -i or --input."  << std::endl;
		printUsage(argv);
		return 1;
	}

	if (!fileExists(options.inputFilename)) {
		std::cerr << "Given input \"" << options.inputFilename << "\" does not exist"  << std::endl;
		return 2;
	}

	std::string passwd = getPasswd();

	std::vector<char> v = gcrypt_decryptFromFile(options.inputFilename, passwd);
	secureErase(passwd);
	if (v.empty()) {
		std::cerr << "Cannot decrypt given input \"" << options.inputFilename << "\" using given password:"  << std::endl;
		std::cerr << gcrypt_getLastErrorString()  << std::endl;
		return 0x80;
	}

	try {
		Document doc(v);
		std::cout << doc.buildXmlString() << std::endl;
	} catch (DocumentException const& e) {
		std::cerr << "Cannot decrypt given input \"" << options.inputFilename << "\" using given password"  << std::endl;
		return 0x81;
	}

	secureErase(v);
	secureErase(passwd);

	return 0;
}


/*{
	std::cout << "Hello, world!"  << std::endl;

	if (argc != 2) {
        fprintf(stderr, "Usage: %s <rsa-keypair.sp>\n", argv[0]);
		std::cerr << "Invalid arguments.";
		return -1;
	}

	passcave::gcrypt_init();

    char* fname = argv[1];
    FILE* lockf = fopen(fname, "wb");
    if (!lockf) {
		std::cerr << "fopen() failed";
		return -1;
    }

	// Generate a new RSA key pair.
    printf("RSA key generation can take a few minutes. Your computer \n"
           "needs to gather random entropy. Please wait... \n\n");

    gcry_error_t err = 0;
    gcry_sexp_t rsa_parms;
    gcry_sexp_t rsa_keypair;

    err = gcry_sexp_build(&rsa_parms, NULL, "(genkey (rsa (nbits 4:2048)))");
    if (err) {
		std::cerr << "gcrypt: failed to create rsa params";
		return -1;
    }

    err = gcry_pk_genkey(&rsa_keypair, rsa_parms);
    if (err) {
		std::cerr << "gcrypt: failed to create rsa key pair";
		return -1;
    }

    printf("RSA key generation complete! Please enter a password to lock \n"
           "your key pair. This password must be committed to memory. \n\n");

	// Grab a key pair password and create an encryption context with it.
    gcry_cipher_hd_t aes_hd;
	passcave::get_aes_ctx(&aes_hd);

	// Encrypt the RSA key pair.
	size_t rsa_len = passcave::get_keypair_size(2048);
    void* rsa_buf = calloc(1, rsa_len);
    if (!rsa_buf) {
		std::cerr << "malloc: could not allocate rsa buffer";
		return -1;
    }
    gcry_sexp_sprint(rsa_keypair, GCRYSEXP_FMT_CANON, rsa_buf, rsa_len);

    err = gcry_cipher_encrypt(aes_hd, (unsigned char*) rsa_buf,
                              rsa_len, NULL, 0);
    if (err) {
		std::cerr << "gcrypt: could not encrypt with AES";
		return -1;
    }

	// Write the encrypted key pair to disk.
    if (fwrite(rsa_buf, rsa_len, 1, lockf) != 1) {
        perror("fwrite");
		std::cerr << "fwrite() failed";
		return -1;
    }

	// Release contexts.
    gcry_sexp_release(rsa_keypair);
    gcry_sexp_release(rsa_parms);
    gcry_cipher_close(aes_hd);
    free(rsa_buf);
    fclose(lockf);

    return 0;
}*/
