/**
 * warrper class for RSA encryption from the course website
 */
#pragma once
#include <osrng.h>
#include <rsa.h>
#include <string>
#include "protocol.h"


static const size_t BITS = 1024;

class RSAPublicWrapper
{
	
public:
	static const size_t KEYSIZE = PUBLIC_KEY_SIZE;

private:
	CryptoPP::AutoSeededRandomPool randomNum;
	CryptoPP::RSA::PublicKey       _publicKey;

public:
	RSAPublicWrapper(const PublicKey& publicKey);
	
	virtual ~RSAPublicWrapper()                                    = default;
	RSAPublicWrapper(const RSAPublicWrapper& other)                = delete;
	RSAPublicWrapper(RSAPublicWrapper&& other) noexcept            = delete;
	RSAPublicWrapper& operator=(const RSAPublicWrapper& other)     = delete;
	RSAPublicWrapper& operator=(RSAPublicWrapper&& other) noexcept = delete;

	std::string encrypt(const uint8_t* plain, size_t length);
};


class RSAPrivateWrapper
{
	
private:
	CryptoPP::AutoSeededRandomPool randomNum;
	CryptoPP::RSA::PrivateKey      _privateKey;


public:
	RSAPrivateWrapper();
	RSAPrivateWrapper(const std::string& key);

	virtual ~RSAPrivateWrapper()                                     = default;
	RSAPrivateWrapper(const RSAPrivateWrapper& other)                = delete;
	RSAPrivateWrapper(RSAPrivateWrapper&& other) noexcept            = delete;
	RSAPrivateWrapper& operator=(const RSAPrivateWrapper& other)     = delete;
	RSAPrivateWrapper& operator=(RSAPrivateWrapper&& other) noexcept = delete;
	
	std::string getPrivateKey() const;
	std::string getPublicKey() const;

	std::string decrypt(const uint8_t* cipher, size_t length);


};
