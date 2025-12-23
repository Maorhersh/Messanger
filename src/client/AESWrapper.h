/**
 * warrper class for AES encryption from the course website
 */
#pragma once
#include <string>
#include "protocol.h"

class AESWrapper
{
public:
	static void GenerateKey(uint8_t* const buffer, const size_t length);

	AESWrapper();
	AESWrapper(const SymmetricKey& symKey);

	virtual ~AESWrapper() = default;
	AESWrapper(const AESWrapper& other) = delete;
	AESWrapper(AESWrapper&& other) noexcept = delete;
	AESWrapper& operator=(const AESWrapper& other) = delete;
	AESWrapper& operator=(AESWrapper&& other) noexcept = delete;

	SymmetricKey getKey() const { return _key; }

	std::string encrypt(const std::string& plain) const;
	std::string encrypt(const uint8_t* plain, size_t length) const;
	std::string decrypt(const uint8_t* cipher, size_t length) const;

private:
	SymmetricKey _key;
};
