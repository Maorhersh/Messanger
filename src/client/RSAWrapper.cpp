/**
 * warrper class for RSA encryption from the course website
 */
#include "RSAWrapper.h"
#include "protocol.h"


RSAPublicWrapper::RSAPublicWrapper(const PublicKey& publicKey)
{
	CryptoPP::StringSource ss((publicKey.publicKey), sizeof(publicKey.publicKey), true);
	_publicKey.Load(ss);
}

std::string RSAPublicWrapper::encrypt(const uint8_t* plain, size_t length)
{
	std::string cipher;
	CryptoPP::RSAES_OAEP_SHA_Encryptor e(_publicKey);
	CryptoPP::StringSource ss(plain, length, true, new CryptoPP::PK_EncryptorFilter(randomNum, e, new CryptoPP::StringSink(cipher)));
	return cipher;
}



RSAPrivateWrapper::RSAPrivateWrapper()
{
	_privateKey.Initialize(randomNum, BITS);
}


RSAPrivateWrapper::RSAPrivateWrapper(const std::string& key)
{
	CryptoPP::StringSource ss(key, true);
	_privateKey.Load(ss);
}

std::string RSAPrivateWrapper::getPrivateKey() const
{
	std::string key;
	CryptoPP::StringSink ss(key);
	_privateKey.Save(ss);
	return key;
}

std::string RSAPrivateWrapper::getPublicKey() const
{
	const CryptoPP::RSAFunction publicKey((_privateKey));
	std::string key;
	CryptoPP::StringSink ss(key);
	publicKey.Save(ss);
	return key;
}


std::string RSAPrivateWrapper::decrypt(const uint8_t* cipher, size_t length)
{
	std::string decrypted;
	CryptoPP::RSAES_OAEP_SHA_Decryptor d(_privateKey);
	CryptoPP::StringSource ss_cipher((cipher), length, true, new CryptoPP::PK_DecryptorFilter(randomNum, d, new CryptoPP::StringSink(decrypted)));
	return decrypted;
}
