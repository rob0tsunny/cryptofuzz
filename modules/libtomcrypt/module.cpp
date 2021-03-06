#include "module.h"
#include <cryptofuzz/util.h>
#include <cryptofuzz/repository.h>
#include <fuzzing/datasource/id.hpp>
#include <tomcrypt.h>

namespace cryptofuzz {
namespace module {

libtomcrypt::libtomcrypt(void) :
    Module("libtomcrypt") {
    CF_CHECK_NE(register_hash(&md2_desc), -1);
    CF_CHECK_NE(register_hash(&md4_desc), -1);
    CF_CHECK_NE(register_hash(&md5_desc), -1);
    CF_CHECK_NE(register_hash(&rmd128_desc), -1);
    CF_CHECK_NE(register_hash(&rmd160_desc), -1);
    CF_CHECK_NE(register_hash(&rmd256_desc), -1);
    CF_CHECK_NE(register_hash(&rmd320_desc), -1);
    CF_CHECK_NE(register_hash(&sha1_desc), -1);
    CF_CHECK_NE(register_hash(&sha224_desc), -1);
    CF_CHECK_NE(register_hash(&sha256_desc), -1);
    CF_CHECK_NE(register_hash(&sha384_desc), -1);
    CF_CHECK_NE(register_hash(&sha512_desc), -1);
    CF_CHECK_NE(register_hash(&tiger_desc), -1);

    return;

end:
    abort();
}

namespace libtomcrypt_detail {
    template <class OperationType, class ReturnType, class CTXType>
    class Operation {
        protected:
            CTXType ctx;
        public:
            Operation(void) { }
            ~Operation() { }
            virtual bool runInit(OperationType& op) = 0;
            virtual bool runUpdate(util::Multipart& parts) = 0;
            virtual std::optional<std::vector<uint8_t>> runFinalize(void) = 0;
            std::optional<ReturnType> Run(OperationType& op) {
                std::optional<ReturnType> ret = std::nullopt;

                Datasource ds(op.modifier.GetPtr(), op.modifier.GetSize());
                util::Multipart parts;

                if ( runInit(op) == false ) {
                    return std::nullopt;
                }

                parts = util::ToParts(ds, op.cleartext);

                CF_CHECK_EQ(runUpdate(parts), true);

                {
                    auto res = runFinalize();
                    if ( res != std::nullopt ) {
                        return ReturnType(res->data(), res->size());
                    }
                }

end:
                return ret;
            }
    };

    template <size_t DigestSize, class CTXType = hash_state>
    class Digest : public Operation<operation::Digest, component::Digest, CTXType> {
        private:
            int (*init)(CTXType*);
            int (*update)(CTXType*, const uint8_t*, unsigned long);
            int (*digest)(CTXType*, uint8_t*);
        public:
            Digest(
                int (*init)(CTXType*),
                int (*update)(CTXType*, const uint8_t*, unsigned long),
                int (*digest)(CTXType*, uint8_t*)
            ) :
                Operation<operation::Digest, component::Digest, CTXType>(),
                init(init),
                update(update),
                digest(digest)
            { }

            bool runInit(operation::Digest& op) override {
                (void)op;
                bool ret = false;

                CF_CHECK_EQ(init(&this->ctx), CRYPT_OK);
                ret = true;

end:
                return ret;
            }

            bool runUpdate(util::Multipart& parts) override {
                bool ret = false;

                for (const auto& part : parts) {
                    if ( part.first == nullptr ) {
                        continue;
                    }
                    CF_CHECK_EQ(update(&this->ctx, part.first, part.second), CRYPT_OK);
                }

                ret = true;

end:
                return ret;
            }

            std::optional<std::vector<uint8_t>> runFinalize(void) override {
                std::optional<std::vector<uint8_t>> ret = std::nullopt;

                std::vector<uint8_t> _ret(DigestSize);
                CF_CHECK_EQ(digest(&this->ctx, _ret.data()), CRYPT_OK);

                ret = _ret;
end:
                return ret;
            }
    };

    static int _crc32_init(crc32_state *ctx) {
        /* noret */ crc32_init(ctx);

        return CRYPT_OK;
    }

    static int _crc32_update(crc32_state *ctx, const unsigned char *input, unsigned long length) {
        /* noret */ crc32_update(ctx, input, length);

        return CRYPT_OK;
    }

    static int _crc32_done(crc32_state* ctx, uint8_t* hash) {
        /* noret */ crc32_finish(ctx, hash, 4);

        return CRYPT_OK;
    }

    static int _adler32_init(adler32_state *ctx) {
        /* noret */ adler32_init(ctx);

        return CRYPT_OK;
    }

    static int _adler32_update(adler32_state *ctx, const unsigned char *input, unsigned long length) {
        /* noret */ adler32_update(ctx, input, length);

        return CRYPT_OK;
    }

    static int _adler32_done(adler32_state* ctx, uint8_t* hash) {
        /* noret */ adler32_finish(ctx, hash, 4);

        return CRYPT_OK;
    }

    Digest<4, crc32_state> crc32(_crc32_init, _crc32_update, _crc32_done);
    Digest<4, adler32_state> adler32(_adler32_init, _adler32_update, _adler32_done);
    Digest<16> md2(md2_init, md2_process, md2_done);
    Digest<16> md4(md4_init, md4_process, md4_done);
    Digest<16> md5(md5_init, md5_process, md5_done);
    Digest<16> ripemd128(rmd128_init, rmd128_process, rmd128_done);
    Digest<20> ripemd160(rmd160_init, rmd160_process, rmd160_done);
    Digest<32> ripemd256(rmd256_init, rmd256_process, rmd256_done);
    Digest<40> ripemd320(rmd320_init, rmd320_process, rmd320_done);
    Digest<20> sha1(sha1_init, sha1_process, sha1_done);
    Digest<28> sha224(sha224_init, sha224_process, sha224_done);
    Digest<32> sha256(sha256_init, sha256_process, sha256_done);
    Digest<48> sha384(sha384_init, sha384_process, sha384_done);
    Digest<64> sha512(sha512_init, sha512_process, sha512_done);
    Digest<24> tiger(tiger_init, tiger_process, tiger_done);
    Digest<20> blake2b160(blake2b_160_init, blake2b_process, blake2b_done);
    Digest<32> blake2b256(blake2b_256_init, blake2b_process, blake2b_done);
    Digest<48> blake2b384(blake2b_384_init, blake2b_process, blake2b_done);
    Digest<64> blake2b512(blake2b_512_init, blake2b_process, blake2b_done);
    Digest<16> blake2s128(blake2s_128_init, blake2s_process, blake2s_done);
    Digest<20> blake2s160(blake2s_160_init, blake2s_process, blake2s_done);
    Digest<28> blake2s224(blake2s_224_init, blake2s_process, blake2s_done);
    Digest<32> blake2s256(blake2s_256_init, blake2s_process, blake2s_done);
#if 0
    Digest<28> keccak224(keccak224_init, keccak224_process, keccak224_done);
    Digest<32> keccak256(keccak256_init, keccak256_process, keccak256_done);
    Digest<48> keccak384(keccak384_init, keccak384_process, keccak384_done);
    Digest<64> keccak512(keccak512_init, keccak512_process, keccak512_done);
#endif

    static int ToHashIdx(const uint64_t digestType) {
        switch ( digestType ) {
            case CF_DIGEST("MD2"):
                return find_hash("md2");
                break;
            case CF_DIGEST("MD4"):
                return find_hash("md4");
                break;
            case CF_DIGEST("MD5"):
                return find_hash("md5");
                break;
            case CF_DIGEST("RIPEMD128"):
                return find_hash("rmd128");
                break;
            case CF_DIGEST("RIPEMD160"):
                return find_hash("rmd160");
                break;
            case CF_DIGEST("RIPEMD256"):
                return find_hash("rmd256");
                break;
            case CF_DIGEST("RIPEMD320"):
                return find_hash("rmd320");
                break;
            case CF_DIGEST("SHA1"):
                return find_hash("sha1");
                break;
            /* Currently disabled because of invalid output */
            //case CF_DIGEST("SHA224"):
            //  return find_hash("sha224");
            //  break;
            case CF_DIGEST("SHA256"):
                return find_hash("sha256");
                break;
            case CF_DIGEST("SHA384"):
                return find_hash("sha384");
                break;
            case CF_DIGEST("SHA512"):
                return find_hash("sha512");
                break;
            case CF_DIGEST("TIGER"):
                return find_hash("tiger");
                break;
            default:
                return -1;
        }
    }
} /* namespace libtomcrypt_detail */

std::optional<component::Digest> libtomcrypt::OpDigest(operation::Digest& op) {
    std::optional<component::Digest> ret = std::nullopt;

    switch ( op.digestType.Get() ) {
        case CF_DIGEST("CRC32"):
            ret = libtomcrypt_detail::crc32.Run(op);
            break;
        case CF_DIGEST("ADLER32"):
            ret = libtomcrypt_detail::adler32.Run(op);
            break;
        case CF_DIGEST("MD2"):
            ret = libtomcrypt_detail::md2.Run(op);
            break;
        case CF_DIGEST("MD4"):
            ret = libtomcrypt_detail::md4.Run(op);
            break;
        case CF_DIGEST("MD5"):
            ret = libtomcrypt_detail::md5.Run(op);
            break;
        case CF_DIGEST("RIPEMD128"):
            ret = libtomcrypt_detail::ripemd128.Run(op);
            break;
        case CF_DIGEST("RIPEMD160"):
            ret = libtomcrypt_detail::ripemd160.Run(op);
            break;
        case CF_DIGEST("RIPEMD256"):
            ret = libtomcrypt_detail::ripemd256.Run(op);
            break;
        case CF_DIGEST("RIPEMD320"):
            ret = libtomcrypt_detail::ripemd320.Run(op);
            break;
        case CF_DIGEST("SHA1"):
            ret = libtomcrypt_detail::sha1.Run(op);
            break;
        case CF_DIGEST("SHA224"):
            ret = libtomcrypt_detail::sha224.Run(op);
            break;
        case CF_DIGEST("SHA256"):
            ret = libtomcrypt_detail::sha256.Run(op);
            break;
        case CF_DIGEST("SHA384"):
            ret = libtomcrypt_detail::sha384.Run(op);
            break;
        case CF_DIGEST("SHA512"):
            ret = libtomcrypt_detail::sha512.Run(op);
            break;
        case CF_DIGEST("TIGER"):
            ret = libtomcrypt_detail::tiger.Run(op);
            break;
        case CF_DIGEST("BLAKE2B160"):
            ret = libtomcrypt_detail::blake2b160.Run(op);
            break;
        case CF_DIGEST("BLAKE2B256"):
            ret = libtomcrypt_detail::blake2b256.Run(op);
            break;
        case CF_DIGEST("BLAKE2B384"):
            ret = libtomcrypt_detail::blake2b384.Run(op);
            break;
        case CF_DIGEST("BLAKE2B512"):
            ret = libtomcrypt_detail::blake2b512.Run(op);
            break;
        case CF_DIGEST("BLAKE2S128"):
            ret = libtomcrypt_detail::blake2s128.Run(op);
            break;
        case CF_DIGEST("BLAKE2S160"):
            ret = libtomcrypt_detail::blake2s160.Run(op);
            break;
        case CF_DIGEST("BLAKE2S224"):
            ret = libtomcrypt_detail::blake2s224.Run(op);
            break;
        case CF_DIGEST("BLAKE2S256"):
            ret = libtomcrypt_detail::blake2s256.Run(op);
            break;
#if 0
        case CF_DIGEST("KECCAK_224"):
            ret = libtomcrypt_detail::keccak224.Run(op);
            break;
        case CF_DIGEST("KECCAK_256"):
            ret = libtomcrypt_detail::keccak256.Run(op);
            break;
        case CF_DIGEST("KECCAK_384"):
            ret = libtomcrypt_detail::keccak384.Run(op);
            break;
        case CF_DIGEST("KECCAK_512"):
            ret = libtomcrypt_detail::keccak512.Run(op);
            break;
#endif
    }

    return ret;
}

std::optional<component::MAC> libtomcrypt::OpHMAC(operation::HMAC& op) {
    std::optional<component::MAC> ret = std::nullopt;

    /* Currently disabled because of invalid output */
    return ret;

    Datasource ds(op.modifier.GetPtr(), op.modifier.GetSize());

    hmac_state ctx;
    uint8_t out[MAXBLOCKSIZE];
    unsigned long outlen;
    std::optional<int> hashIdx = std::nullopt;
    util::Multipart parts;

    CF_CHECK_NE(op.cipher.key.GetPtr(), nullptr);

    CF_CHECK_NE(hashIdx = libtomcrypt_detail::ToHashIdx(op.digestType.Get()), std::nullopt);

    CF_CHECK_EQ(hmac_init(&ctx, *hashIdx, op.cipher.key.GetPtr(), op.cipher.key.GetSize()), CRYPT_OK);

    parts = util::ToParts(ds, op.cleartext);
    for (const auto& part : parts) {
        if ( part.first == nullptr ) {
            continue;
        }
        CF_CHECK_EQ(hmac_process(&ctx, part.first, part.second), CRYPT_OK);
    }

    CF_CHECK_EQ(hmac_done(&ctx, out, &outlen), CRYPT_OK);

    ret = component::MAC(out, outlen);
end:
    return ret;
}

std::optional<component::Key> libtomcrypt::OpKDF_HKDF(operation::KDF_HKDF& op) {
    std::optional<component::Key> ret = std::nullopt;
    std::optional<int> hashIdx = std::nullopt;
    uint8_t* out = util::malloc(op.keySize);

    CF_CHECK_NE(out, nullptr);
    CF_CHECK_NE(op.password.GetPtr(), nullptr);

    CF_CHECK_NE(hashIdx = libtomcrypt_detail::ToHashIdx(op.digestType.Get()), std::nullopt);

    CF_CHECK_EQ(hkdf(
                *hashIdx,
                op.salt.GetPtr(),
                op.salt.GetSize(),
                op.info.GetPtr(),
                op.info.GetSize(),
                op.password.GetPtr(),
                op.password.GetSize(),
                out,
                op.keySize), CRYPT_OK);

    ret = component::Key(out, op.keySize);

end:
    util::free(out);

    return ret;
}

std::optional<component::Key> libtomcrypt::OpKDF_BCRYPT(operation::KDF_BCRYPT& op) {
    std::optional<component::Key> ret = std::nullopt;

    /* bcrypt currently disabled because it leads to OOMs */
    return ret;

    std::optional<int> hashIdx = std::nullopt;
    uint8_t* out = util::malloc(op.keySize);

    CF_CHECK_NE(out, nullptr);
    CF_CHECK_NE(op.iterations, 0);
    CF_CHECK_NE(op.secret.GetPtr(), nullptr);
    CF_CHECK_NE(op.salt.GetPtr(), nullptr);

    CF_CHECK_NE(hashIdx = libtomcrypt_detail::ToHashIdx(op.digestType.Get()), std::nullopt);

    {
        unsigned long outLen = op.keySize;
        CF_CHECK_EQ(bcrypt_pbkdf_openbsd(
                    op.secret.GetPtr(),
                    op.secret.GetSize(),
                    op.salt.GetPtr(),
                    op.salt.GetSize(),
                    op.iterations,
                    *hashIdx,
                    out,
                    &outLen), CRYPT_OK);

        ret = component::Key(out, outLen);
    }

end:
    util::free(out);

    return ret;
}

} /* namespace module */
} /* namespace cryptofuzz */
