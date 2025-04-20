
#include <assert.h>
#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/ec.h>
#include <openssl/rsa.h>
#include <openssl/dh.h>
#include <openssl/ecdsa.h>
#include <openssl/objects.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <zlib.h>
#include <set> 
#include <vector>
#include <string>
#include <map>
#include <functional> 

#include <pch1.h>

#include "ecc_sv.h"

//#include <mem_pe.h>

//#include <nlohmann/json.hpp>
//
//#if defined( NLOHMANN_JSON_HPP) || defined(INCLUDE_NLOHMANN_JSON_HPP_)
//using njson = nlohmann::json;
//#define NJSON_H
//#endif

#include "mapView.h"

#ifdef max
#undef max
#undef min
#endif
#if 0
//def _WIN32
#include <Bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#endif // _WIN32

#ifndef _WIN32
//#include <base/curses.h>
#include <termio.h>
int getch(void)
{
	struct termios tm, tm_old;
	int fd = 0, ch;

	if (tcgetattr(fd, &tm) < 0) {//保存现在的终端设置
		return -1;
	}

	tm_old = tm;
	cfmakeraw(&tm);//更改终端设置为原始模式，该模式下所有的输入数据以字节为单位被处理
	if (tcsetattr(fd, TCSANOW, &tm) < 0) {//设置上更改之后的设置
		return -1;
	}

	ch = getchar();
	if (tcsetattr(fd, TCSANOW, &tm_old) < 0) {//更改设置为最初的样子
		return -1;
	}
	return ch;
}
#else
#include <conio.h>
#endif

#if 0
#define SSL_CBD(n,f,f2) f(*n)f2
struct ssl_cbs
{
	// bio.h
	SSL_CBD(BIO_number_written, uint64_t, (BIO* bio));
	SSL_CBD(BIO_new, BIO*, (const BIO_METHOD* type));
	SSL_CBD(BIO_free, int, (BIO* a));
	SSL_CBD(BIO_read, int, (BIO* b, void* data, int dlen));
	SSL_CBD(BIO_write, int, (BIO* b, const void* data, int dlen));
	SSL_CBD(BIO_ctrl, int, (BIO* bp, int cmd, long larg, void* parg));
	SSL_CBD(BIO_free_all, void, (BIO* a));
	SSL_CBD(BIO_s_mem, const BIO_METHOD*, (void));
	SSL_CBD(BIO_new_mem_buf, BIO*, (const void* buf, int len));
	// bn.h
	SSL_CBD(BN_CTX_new, BN_CTX*, ());
	SSL_CBD(BN_CTX_free, void, (BN_CTX* c));
	SSL_CBD(BN_num_bits, int, (const BIGNUM* a));
	SSL_CBD(BN_new, BIGNUM*, (void));
	SSL_CBD(BN_copy, BIGNUM*, (BIGNUM* a, const BIGNUM* b));
	SSL_CBD(BN_bin2bn, BIGNUM*, (const unsigned char* s, int len, BIGNUM* ret));
	SSL_CBD(BN_bn2bin, int, (const BIGNUM* a, unsigned char* to));
	SSL_CBD(BN_add, int, (BIGNUM* r, const BIGNUM* a, const BIGNUM* b));
	SSL_CBD(BN_mul, int, (BIGNUM* r, const BIGNUM* a, const BIGNUM* b, BN_CTX* ctx));
	SSL_CBD(BN_div, int, (BIGNUM* dv, BIGNUM* rem, const BIGNUM* m, const BIGNUM* d, BN_CTX* ctx));
	SSL_CBD(BN_mul_word, int, (BIGNUM* a, BN_ULONG w));
	SSL_CBD(BN_set_word, int, (BIGNUM* a, BN_ULONG w));
	SSL_CBD(BN_get_word, BN_ULONG, (const BIGNUM* a));
	SSL_CBD(BN_cmp, int, (const BIGNUM* a, const BIGNUM* b));
	SSL_CBD(BN_free, void, (BIGNUM* a));
	SSL_CBD(BN_bn2hex, char*, (const BIGNUM* a));
	SSL_CBD(BN_bn2dec, char*, (const BIGNUM* a));
	SSL_CBD(BN_hex2bn, int, (BIGNUM** a, const char* str));
	SSL_CBD(BN_dec2bn, int, (BIGNUM** a, const char* str));
	SSL_CBD(BN_asc2bn, int, (BIGNUM** a, const char* str));
	SSL_CBD(BN_gcd, int, (BIGNUM* r, const BIGNUM* a, const BIGNUM* b, BN_CTX* ctx));

	SSL_CBD(EC_GROUP_order_bits, int, (const EC_GROUP* group));
	SSL_CBD(EC_GROUP_get_curve_name, int, (const EC_GROUP* group));
	SSL_CBD(EC_get_builtin_curves, size_t, (EC_builtin_curve* r, size_t nitems));
	SSL_CBD(EC_POINT_new, EC_POINT*, (const EC_GROUP* group));
	SSL_CBD(EC_POINT_free, void, (EC_POINT*));
	SSL_CBD(EC_POINT_point2oct, size_t, (const EC_GROUP* group, const EC_POINT* p, point_conversion_form_t form, unsigned char* buf, size_t len, BN_CTX* ctx));
	SSL_CBD(EC_POINT_oct2point, int, (const EC_GROUP* group, EC_POINT* p, const unsigned char* buf, size_t len, BN_CTX* ctx));
	SSL_CBD(EC_POINT_mul, int, (const EC_GROUP* group, EC_POINT* r, const BIGNUM* n, const EC_POINT* q, const BIGNUM* m, BN_CTX* ctx));
	SSL_CBD(EC_KEY_new_by_curve_name, EC_KEY*, (int nid));
	SSL_CBD(EC_KEY_free, void, (EC_KEY* key));
	SSL_CBD(EC_KEY_get0_group, const EC_GROUP*, (const EC_KEY* key));
	SSL_CBD(EC_KEY_get0_private_key, const BIGNUM*, (const EC_KEY* key));
	SSL_CBD(EC_KEY_get0_public_key, const EC_POINT*, (const EC_KEY* key));
	SSL_CBD(EC_KEY_set_public_key, int, (EC_KEY* key, const EC_POINT* pub));
	SSL_CBD(EC_KEY_generate_key, int, (EC_KEY* key));
	SSL_CBD(EC_KEY_oct2key, int, (EC_KEY* key, const unsigned char* buf, size_t len, BN_CTX* ctx));
	SSL_CBD(EC_KEY_oct2priv, int, (EC_KEY* key, const unsigned char* buf, size_t len));
	SSL_CBD(EC_KEY_priv2oct, size_t, (const EC_KEY* key, unsigned char* buf, size_t len));
	SSL_CBD(d2i_ECPrivateKey, EC_KEY*, (EC_KEY** key, const unsigned char** in, long len));
	SSL_CBD(i2d_ECPrivateKey, int, (EC_KEY* key, unsigned char** out));
	SSL_CBD(d2i_ECParameters, EC_KEY*, (EC_KEY** key, const unsigned char** in, long len));
	SSL_CBD(i2d_ECParameters, int, (EC_KEY* key, unsigned char** out));
	SSL_CBD(i2o_ECPublicKey, int, (const EC_KEY* key, unsigned char** out););
	SSL_CBD(ECDH_compute_key, int, (void* out, size_t outlen, const EC_POINT* pub_key, const EC_KEY* ecdh, void* (*KDF) (const void* in, size_t inlen, void* out, size_t* outlen)));
	SSL_CBD(ECDSA_SIG_free, void, (ECDSA_SIG* sig));
	SSL_CBD(i2d_ECDSA_SIG, int, (const ECDSA_SIG* sig, unsigned char** pp));
	SSL_CBD(ECDSA_do_sign, ECDSA_SIG*, (const unsigned char* dgst, int dgst_len, EC_KEY* eckey));
	SSL_CBD(ECDSA_verify, int, (int type, const unsigned char* dgst, int dgstlen, const unsigned char* sig, int siglen, EC_KEY* eckey));
	// rsa.h
	SSL_CBD(RSA_generate_key, RSA*, (int bits, unsigned long e, void(*callback) (int, int, void*), void* cb_arg));
	SSL_CBD(RSA_free, void, (RSA* r));
	SSL_CBD(RSA_sign, int, (int type, const unsigned char* m, unsigned int m_length, unsigned char* sigret, unsigned int* siglen, RSA* rsa));
	SSL_CBD(RSA_verify, int, (int type, const unsigned char* m, unsigned int m_length, const unsigned char* sigbuf, unsigned int siglen, RSA* rsa));
	// objects.h
	SSL_CBD(OBJ_nid2sn, const char*, (int n));
	SSL_CBD(OBJ_sn2nid, int, (const char* s));
	// evp.h
	SSL_CBD(EVP_CIPHER_iv_length, int, (const EVP_CIPHER* cipher));
	SSL_CBD(EVP_MD_CTX_new, EVP_MD_CTX*, (void));
	SSL_CBD(EVP_MD_CTX_free, void, (EVP_MD_CTX* ctx));
	SSL_CBD(EVP_DigestInit_ex, int, (EVP_MD_CTX* ctx, const EVP_MD* type, ENGINE* impl));
	SSL_CBD(EVP_DigestUpdate, int, (EVP_MD_CTX* ctx, const void* d, size_t cnt));
	SSL_CBD(EVP_DigestFinal_ex, int, (EVP_MD_CTX* ctx, unsigned char* md, unsigned int* s));
	SSL_CBD(EVP_DigestInit, int, (EVP_MD_CTX* ctx, const EVP_MD* type));
	SSL_CBD(EVP_DigestFinal, int, (EVP_MD_CTX* ctx, unsigned char* md, unsigned int* s));
	SSL_CBD(EVP_BytesToKey, int, (const EVP_CIPHER* type, const EVP_MD* md,
		const unsigned char* salt,
		const unsigned char* data, int datal, int count,
		unsigned char* key, unsigned char* iv));
	SSL_CBD(EVP_CipherInit_ex, int, (EVP_CIPHER_CTX* ctx,
		const EVP_CIPHER* cipher, ENGINE* impl,
		const unsigned char* key,
		const unsigned char* iv, int enc));
	SSL_CBD(EVP_CipherUpdate, int, (EVP_CIPHER_CTX* ctx, unsigned char* out,
		int* outl, const unsigned char* in, int inl));
	SSL_CBD(EVP_CipherFinal_ex, int, (EVP_CIPHER_CTX* ctx, unsigned char* outm,
		int* outl));
	SSL_CBD(EVP_CIPHER_CTX_new, EVP_CIPHER_CTX*, (void));
	SSL_CBD(EVP_CIPHER_CTX_free, void, (EVP_CIPHER_CTX* c));
	SSL_CBD(EVP_md5, const EVP_MD*, (void));
	SSL_CBD(EVP_sha256, const EVP_MD*, (void));
	SSL_CBD(EVP_aes_256_cbc, const EVP_CIPHER*, (void));
	SSL_CBD(EVP_CIPHER_do_all, void, (void (*fn) (const EVP_CIPHER* ciph,
		const char* from, const char* to, void* x),
		void* arg));
	SSL_CBD(EVP_MD_do_all, void, (void (*fn) (const EVP_MD* ciph,
		const char* from, const char* to, void* x),
		void* arg)
	);
	// pem.h
	SSL_CBD(PEM_write_bio_RSAPrivateKey, int, (BIO* bp, RSA* x, const EVP_CIPHER*, unsigned char* kstr, int klen, pem_password_cb* cb, void* u));
	SSL_CBD(PEM_read_bio_RSAPublicKey, RSA*, (BIO* bp, RSA** x, pem_password_cb* cb, void* u));
	SSL_CBD(PEM_write_bio_RSAPublicKey, int, (BIO* bp, const RSA*));
	SSL_CBD(PEM_write_bio_ECPKParameters, int, (BIO* bp, const EC_GROUP* x));
	SSL_CBD(PEM_read_bio_ECPrivateKey, EC_KEY*, (BIO* bp, EC_KEY** x, pem_password_cb* cb, void* u));
	SSL_CBD(PEM_write_bio_ECPrivateKey, int, (BIO* bp, EC_KEY* x, const EVP_CIPHER*, unsigned char* kstr, int klen, pem_password_cb* cb, void* u));
	SSL_CBD(PEM_read_bio_EC_PUBKEY, EC_KEY*, (BIO* bp, EC_KEY** x, pem_password_cb* cb, void* u));
	SSL_CBD(PEM_write_bio_EC_PUBKEY, int, (BIO* bp, EC_KEY* x));
	SSL_CBD(RAND_bytes, int, (unsigned char* buf, int num));


};
#undef SSL_CBD
#ifdef _WIN32 

#define cNAME_0 "libcrypto-3"
#define sNAME_0 "libssl-3" 

#else
#define cNAME_0 "libcrypto.so" 
#define sNAME_0 "libssl.so" 
#endif // _WIN32

class ssl_lib_cx :public ssl_cbs
{
public:
	hz::Shared* c0 = {}, * s0 = {};
	std::map<std::string, const void*> _ciphers;
	std::vector<std::string> _ciphers_n;
public:
	ssl_lib_cx();
	~ssl_lib_cx();

	void load_ssl();
private:

};

MCPP_EXPORT void ztestssl()
{
	BIO_f_ssl();
	auto mdctx = EVP_MD_CTX_new();
}
ssl_lib_cx::ssl_lib_cx()
{
	load_ssl();
}

ssl_lib_cx::~ssl_lib_cx()
{
	if (s0)
		delete s0; s0 = 0;
	if (c0)
		delete c0; c0 = 0;
}

void ssl_lib_cx::load_ssl()
{
	return;
	std::string cso = cNAME_0;
	std::string sso = sNAME_0;
#ifdef _WIN32 
	if (sizeof(void*) == 8)
	{
		cso += "-x64.dll";
		sso += "-x64.dll";
	}
	else {
		cso += ".dll";
		sso += ".dll";
	}
#endif
	do {
		if (!c0)
			c0 = hz::Shared::loadShared(cso);
		if (!s0)
			s0 = hz::Shared::loadShared(sso);
		if (!c0 || !s0)break;
		void* nc = 0;
#define SSL_CBD(n)  nc=c0->get_cb(#n, (void**)(&this->n)) ? (void*)this->n : s0->get_cb(#n, (void**)(&this->n))
		SSL_CBD(BIO_number_written);
		SSL_CBD(BIO_new);
		SSL_CBD(BIO_free);
		SSL_CBD(BIO_read);
		SSL_CBD(BIO_write);
		SSL_CBD(BIO_ctrl);
		SSL_CBD(BIO_free_all);
		SSL_CBD(BIO_s_mem);
		SSL_CBD(BIO_new_mem_buf);
		SSL_CBD(BN_CTX_new);
		SSL_CBD(BN_CTX_free);
		SSL_CBD(BN_num_bits);
		SSL_CBD(BN_new);
		SSL_CBD(BN_copy);
		SSL_CBD(BN_bin2bn);
		SSL_CBD(BN_bn2bin);
		SSL_CBD(BN_add);
		SSL_CBD(BN_mul);
		SSL_CBD(BN_div);
		SSL_CBD(BN_mul_word);
		SSL_CBD(BN_set_word);
		SSL_CBD(BN_get_word);
		SSL_CBD(BN_cmp);
		SSL_CBD(BN_free);
		SSL_CBD(BN_bn2hex);
		SSL_CBD(BN_bn2dec);
		SSL_CBD(BN_hex2bn);
		SSL_CBD(BN_dec2bn);
		SSL_CBD(BN_asc2bn);
		SSL_CBD(BN_gcd);
		SSL_CBD(EC_GROUP_order_bits);
		SSL_CBD(EC_GROUP_get_curve_name);
		SSL_CBD(EC_get_builtin_curves);
		SSL_CBD(EC_POINT_new);
		SSL_CBD(EC_POINT_free);
		SSL_CBD(EC_POINT_point2oct);
		SSL_CBD(EC_POINT_oct2point);
		SSL_CBD(EC_POINT_mul);
		SSL_CBD(EC_KEY_new_by_curve_name);
		SSL_CBD(EC_KEY_free);
		SSL_CBD(EC_KEY_get0_group);
		SSL_CBD(EC_KEY_get0_private_key);
		SSL_CBD(EC_KEY_get0_public_key);
		SSL_CBD(EC_KEY_set_public_key);
		SSL_CBD(EC_KEY_generate_key);
		SSL_CBD(EC_KEY_oct2key);
		SSL_CBD(EC_KEY_oct2priv);
		SSL_CBD(EC_KEY_priv2oct);
		SSL_CBD(d2i_ECPrivateKey);
		SSL_CBD(i2d_ECPrivateKey);
		SSL_CBD(d2i_ECParameters);
		SSL_CBD(i2d_ECParameters);
		SSL_CBD(i2o_ECPublicKey);
		SSL_CBD(ECDH_compute_key);
		SSL_CBD(ECDSA_SIG_free);
		SSL_CBD(i2d_ECDSA_SIG);
		SSL_CBD(ECDSA_do_sign);
		SSL_CBD(ECDSA_verify);
		SSL_CBD(RSA_generate_key);
		SSL_CBD(RSA_free);
		SSL_CBD(RSA_sign);
		SSL_CBD(RSA_verify);
		SSL_CBD(OBJ_nid2sn);
		SSL_CBD(OBJ_sn2nid);
		SSL_CBD(EVP_CIPHER_iv_length);
		SSL_CBD(EVP_MD_CTX_new);
		SSL_CBD(EVP_MD_CTX_free);
		SSL_CBD(EVP_DigestInit_ex);
		SSL_CBD(EVP_DigestUpdate);
		SSL_CBD(EVP_DigestFinal_ex);
		SSL_CBD(EVP_DigestInit);
		SSL_CBD(EVP_DigestFinal);
		SSL_CBD(EVP_BytesToKey);
		SSL_CBD(EVP_CipherInit_ex);
		SSL_CBD(EVP_CipherUpdate);
		SSL_CBD(EVP_CipherFinal_ex);
		SSL_CBD(EVP_CIPHER_CTX_new);
		SSL_CBD(EVP_CIPHER_CTX_free);
		SSL_CBD(EVP_md5);
		SSL_CBD(EVP_sha256);
		SSL_CBD(EVP_aes_256_cbc);
		SSL_CBD(EVP_CIPHER_do_all);
		SSL_CBD(EVP_MD_do_all);
		SSL_CBD(PEM_write_bio_RSAPrivateKey);
		SSL_CBD(PEM_read_bio_RSAPublicKey);
		SSL_CBD(PEM_write_bio_RSAPublicKey);
		SSL_CBD(PEM_write_bio_ECPKParameters);
		SSL_CBD(PEM_read_bio_ECPrivateKey);
		SSL_CBD(PEM_write_bio_ECPrivateKey);
		SSL_CBD(PEM_read_bio_EC_PUBKEY);
		SSL_CBD(PEM_write_bio_EC_PUBKEY);
		SSL_CBD(RAND_bytes);
	} while (0);
}
static ssl_lib_cx sctx;
#endif // 1


namespace hz
{
	class ecc_c
	{
	private:
		void* eckey = nullptr; //EC_KEY* eckey = nullptr;
		int _nid = 0, erc = 0;
		std::string _pubkey, _private_key;
		std::string _shared;
		std::function<int(char* buf, int size, int rwflag)> _pass_cb;// pem加密回调
	public:
		ecc_c();
		ecc_c(int nid);
		~ecc_c();
		std::string get_pubkey1(bool isbstr = false);
		std::string get_prikey1();
		std::string compute_key(const std::string& pubkey2);
		std::string get_shared();
		std::string get_pubkey();
		std::string get_prikey();
		// 保存蜜钥到文件
		std::string save_key2json(const std::string& fn);
		// 从私钥文件初始化
		void load_key2file(const std::string& fn);
		// 初始化新，默认 NID_X9_62_prime256v1;
		void init(int nid = -1);
		void init(const std::string& parameters);
		bool empty();
		// 创建新密钥
		void new_key(int nid = -1/* NID_secp256k1*/);
		void new_key(const char* sn);
		// 从私钥数据初始化，公钥可为空自动创建
		void set_prikey(const std::string& prikey, const std::string& parameters, int nid);
		void set_pubkey(const std::string& pubkey, const std::string& parameters, int nid);
		// 从pem创建私钥或公钥
		bool pem_mem(const std::string& pemdata, const std::string& pass = "");
		bool pem_mem_cb(const std::string& pemdata, std::function<int(char* buf, int size, int rwflag)> cb);
		// 获取pem数据,t=0private,t=1public
		std::string get_pem(int t, const std::string& kstr);
		// 保存到pem到指针
		void save_pem(std::string* _private, std::string* _pub, const std::string& pass);
		// 创建公钥
		void gen_pubkey();
		std::string do_sign(const std::string& dgst);
		// returns 1: correct signature  0: incorrect signature	 -1: error
		bool do_verify(const std::string& dgst, const std::string& sig);
	public:
		void swap(ecc_c& c);
	public:
		// 获取加密算法列表
		static std::map<std::string, const void*> get_ciphers();
		// 加密函数
		static bool cipher_encrypt(const void* cipher, const void* data, int size, const std::string& key, std::string& outd, std::string& iv, int ps);
		static bool cipher_decrypt(const void* cipher, const void* data, int size, const std::string& key, std::string& outd, const std::string& iv);

		static std::string randstr(int count);

		static bool decrypt(const void* cipher, const char* data, size_t size, const std::string& sharedkey, std::string& out);
		static bool encrypt(const void* cipher, const char* data, size_t size, const std::string& sharedkey, std::string& out);

		// 创建随机iv，密钥取32字节
		static bool encrypt_iv0(const void* ecp, const char* data, size_t size, const std::string& sharedkey, std::string& out);
		static bool decrypt_iv0(const void* ecp, const char* data, size_t size, const std::string& sharedkey, std::string& out);

		static std::string b58_encode(const std::string& source);
		static std::string b58_decode(const std::string& str);
		static std::string b64_encode(const std::string& source);
		static std::string b64_decode(const std::string& str);
		// 获取所有支持的曲线
		static std::map<int, const char*> get_curve_names();
		static std::map<const char*, int> get_curve_names1();
		// 获取摘要算法列表
		static std::map<std::string, const void*> get_mds();
		// 计算摘要
		static std::string sha256(const void* data, size_t len);
		static std::string do_dgst(const void* vmd, const char* data, size_t len, const std::string& salt = "", int count = 1, int out_size = 0);
		static std::string do_md(const void* vmd, const std::string& data);
		static std::string do_md(const void* vmd, const char* data, size_t len);
		static std::string crc32s(const void* data, size_t len);
		static uint32_t crc32u(const void* data, size_t len);
		// 私钥签名
		static std::string pri_sign(const std::string& prikey, const std::string& dgst, const std::string& pass, std::string* pubkey = nullptr);
		// 公钥验证
		static bool pub_verify(const std::string& dgst, const std::string& sig, const std::string& pubkey);
		static bool pub_verify1(const std::string& dgst, const std::string& sig, const std::string& pubkey, int nid);
	public:
		static void disp(const char* str, const void* pbuf, const int size);
		static std::string bin2hex(const char* str, int size, bool is_lower);
		static std::string ptr2hex(const void* p, bool is_lower);
		static std::string hex2bin(const char* str, int size);
		// pem公钥数据转pub hex
		static std::string pem2pub(const std::string& pubstr);
	private:
		static int pem_password_cbs(char* buf, int size, int rwflag, void* userdata);
		static int pem_pass_cbs(char* buf, int size, int rwflag, void* userdata);
	};
	class rsa_c
	{
	public:
		void* rkey = 0;
		std::string pri_key;           // Private key
		std::string pub_key;           // Public key
	public:
		rsa_c();
		~rsa_c();
		std::string genrsa(int w);
		void set_pubkey(const std::string& k);
		std::string sign(const std::string& dgst);
		bool verify(const std::string& dgst, const std::string& sig);
	};

	// 解码base85
	void decode85(const unsigned char* src, unsigned char* dst);
	std::string c2md(const void* data, int len, const void* md);


	// 用nid创建一个新的ecckey 
	void* new_ecckey_nid(int nid);
	// 从pem数据创建ecckey。支持从get_ecckey_pem获取的数据创建。私钥pem或公钥pem数据 。密码可选
	void* new_ecckey_pem(const char* pem, const char* pass);
	// 获取pem数据方便保存。
	std::string get_ecckey_pem(void* ecckey, const std::string& kstr);
	// 获取公钥
	std::string get_ecckey_pem_public(void* ecckey);
	// 获取共享密钥。输入我方的私钥，对方的公钥，参数使用new_ecckey_pem返回的指针，私钥也可以用new_ecckey_nid返回的指针
	std::string get_compute_key(void* ecckey, void* pubkey);
	// 私钥签名
	std::string ecprivatekey_sign(void* prikey, const std::string& dgst, std::string* pubkey = nullptr);
	// 用公钥验证
	bool public_verify(void* pubkey, const std::string& dgst, const std::string& sig);

}// !hz







#if 1

// return < 0失败
int do_cipher(cipher_data_t* ct)
{
	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	int i = ct->dsize, j = 0, ret = 0;
	unsigned char* p = (unsigned char*)(ct->outd ? ct->outd : ct->data), * data = (unsigned char*)ct->data;
	unsigned char key[EVP_MAX_KEY_LENGTH] = {};

	if (!ct->cipher)
	{
		return -4;
	}
	auto cipher = (const EVP_CIPHER*)ct->cipher;
	auto iv = ct->iv;
	if (!ct->ivlen)
	{
		ct->ivlen = EVP_CIPHER_iv_length(cipher);
		if (RAND_bytes(iv, ct->ivlen) <= 0) /* Generate a salt */
			ret = -4;
	}
	if (ct->rawkey) {
		if (ct->klen == EVP_MAX_KEY_LENGTH)
		{
			memcpy(key, ct->kstr, ct->klen);
		}
		else
		{
			int klen = strlen(ct->kstr);
			// 使用md5计算密钥
			if (!EVP_BytesToKey(cipher, EVP_md5(), iv, (unsigned char*)ct->kstr, klen, 1, key, NULL))
				ret = -2;
		}
	}
	else {
		if (ct->klen > EVP_MAX_KEY_LENGTH)
			ct->klen = EVP_MAX_KEY_LENGTH;
		memcpy(key, ct->kstr, ct->klen);
	}
	size_t ens = 0;
	do {
		if (ret < 0 || !ctx)break;
		int k = EVP_CipherInit_ex(ctx, cipher, NULL, key, iv, ct->enc);
		k += EVP_CipherUpdate(ctx, p, &j, data, i);
		ens += j;
		p += ens;
		k += EVP_CipherFinal_ex(ctx, p, &i);
		ens += i;
		if (k < 2)
			ret = -1;
	} while (0);
	if (ret == 0)
	{
		ret = ens;
		ct->out_size = ret;
	}
	EVP_CIPHER_CTX_free(ctx);
	return ret;
}



#endif // 1

namespace hz
{
	int save_file(const char* path, std::string& d);
	int read_file(const char* path, std::string& d);


#ifndef nobase58
	//编码表
	static const char b58_ordered[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
	static const char b58idx0 = '1';
	static const char b64_ordered[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#if 0
	std::string b58enc(const void* data, size_t binsz)
	{
		std::string ret;
		int carry;
		size_t i, j, high, zcount = 0;
		const uint8_t* bin = (uint8_t*)data;
		std::vector<uint8_t> buf;
		size_t size;

		//计算待编码数据前面 0 的个数
		while (zcount < (size_t)binsz && !bin[zcount])
			++zcount;

		//计算存放转换数据所需要的数组的大小    138/100 --> log(256)/log(58)
		size = (binsz - zcount) * 138 / 100 + 1;
		buf.resize(size);
		//遍历待转换数据
		for (i = zcount, high = size - 1; i < (size_t)binsz; ++i, high = j)
		{
			//将数据从后往前依次存放
			for (carry = bin[i], j = size - 1; (j > high) || carry; --j)
			{
				carry += 256 * buf[j];
				buf[j] = carry % 58;
				carry /= 58;
			}
		}

		for (j = 0; j < (size_t)size && !buf[j]; ++j);

		ret.assign('1', zcount);
		ret.resize(zcount + size - j);

		for (i = zcount; j < (size_t)size; ++i, ++j)
			ret[i] = b58digits_ordered[buf[j]];

		return ret;
	}
	static const int8_t b58digits_map[] = {
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1,
		-1, 9, 10, 11, 12, 13, 14, 15, 16, -1, 17, 18, 19, 20, 21, -1,
		22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, -1, -1, -1, -1,
		-1, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, -1, 44, 45, 46,
		47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, -1, -1, -1, -1, -1,
	};
	std::string b58dec(const char* b58)
	{
		std::string ret;
		//void* bin, size_t* binszp,
		size_t binsz = *binszp;
		const unsigned char* b58u = (const unsigned char*)b58;
		unsigned char* binu = bin;

		uint32_t outi[12 * 1024] = { 0 };
		size_t outisz = (binsz + 3) / 4;

		uint64_t t;
		uint32_t c;
		size_t i, j;
		uint8_t bytesleft = binsz % 4;
		uint32_t zeromask = bytesleft ? (0xffffffff << (bytesleft * 8)) : 0;
		unsigned zerocount = 0;
		size_t b58sz;

		b58sz = strlen(b58);

		memset(outi, 0, outisz * sizeof(*outi));

		// Leading zeros, just count
		for (i = 0; i < b58sz && b58u[i] == '1'; ++i)
			++zerocount;

		for (; i < b58sz; ++i)
		{
			if (b58u[i] & 0x80)
				// High-bit set on invalid digit
				return false;
			if (b58digits_map[b58u[i]] == -1)
				// Invalid base58 digit
				return false;
			c = (unsigned)b58digits_map[b58u[i]];
			for (j = outisz; j--; )
			{
				t = ((uint64_t)outi[j]) * 58 + c;
				c = (t & 0x3f00000000) >> 32;
				outi[j] = t & 0xffffffff;
			}
			if (c)
				// Output number too big (carry to the next int32)
				return false;
			if (outi[0] & zeromask)
				// Output number too big (last int32 filled too far)
				return false;
		}

		j = 0;
		switch (bytesleft) {
		case 3:
			*(binu++) = (outi[0] & 0xff0000) >> 16;
			//-fallthrough
		case 2:
			*(binu++) = (outi[0] & 0xff00) >> 8;
			//-fallthrough
		case 1:
			*(binu++) = (outi[0] & 0xff);
			++j;
			//-fallthrough
		default:
			break;
		}

		for (; j < outisz; ++j)
		{
			*(binu++) = (outi[j] >> 0x18) & 0xff;
			*(binu++) = (outi[j] >> 0x10) & 0xff;
			*(binu++) = (outi[j] >> 8) & 0xff;
			*(binu++) = (outi[j] >> 0) & 0xff;
		}

		// Count canonical base58 byte count
		binu = bin;
		for (i = 0; i < binsz; ++i)
		{
			if (binu[i]) {
				if (zerocount > i) {
					/* result too large */
					return false;
				}
				break;
			}
			-- * binszp;
		}
		*binszp += zerocount;

		return true;
	}
#endif
	class base
	{
	private:
		std::string ALPHABET;
		std::map<char, int> ALPHABET_MAP = {};
		int BASE = 0;
		char LEADER = 0;
	public:
		base()
		{
		}

		base(const std::string& alphabet) {
			ALPHABET = alphabet;
			BASE = ALPHABET.size();
			LEADER = ALPHABET.at(0);
			// pre-compute lookup table
			for (int z = 0; z < ALPHABET.size(); z++) {
				char x = ALPHABET.at(z);
				if (ALPHABET_MAP.find(x) != ALPHABET_MAP.end()) throw (std::string(x, 1) + " is ambiguous");
				ALPHABET_MAP[x] = z;
			}
		}
		~base()
		{
		}
	public:


		std::string encode(const std::string& source) {
			if (source.empty()) return "";

			std::vector<int> digits;
			digits.push_back(0);
			unsigned int carry;
			for (int i = 0; i < source.size(); ++i) {
				carry = source[i];
				for (int j = 0; j < digits.size(); ++j) {
					carry += digits[j] << 8;
					digits[j] = carry % BASE;
					carry = (carry / BASE) | 0;
				}

				while (carry > 0) {
					digits.push_back(carry % BASE);
					carry = (carry / BASE) | 0;
				}
			}

			std::string str;
			str.reserve(source.size() + digits.size());
			// deal with leading zeros
			for (int k = 0; source[k] == 0 && k < source.size() - 1; ++k) str.push_back(ALPHABET[0]);
			// convert digits to a string
			for (int q = digits.size() - 1; q >= 0; --q) str.push_back(ALPHABET[digits[q]]);

			return str;
		}

		std::string decode(const std::string& str) {
			if (str.empty()) return "";

			std::string bytes;
			bytes.push_back(0);
			for (int i = 0; i < str.size(); i++) {
				auto value = ALPHABET_MAP[str[i]];
				//if (value == 0) return "";
				int j;
				unsigned int carry;
				for (j = 0, carry = value; j < bytes.size(); ++j) {
					carry += bytes[j] * BASE;
					bytes[j] = carry & 0xff;
					carry >>= 8;
				}

				while (carry > 0) {
					bytes.push_back(carry & 0xff);
					carry >>= 8;
				}
			}

			// deal with leading zeros
			for (int k = 0; str[k] == LEADER && k < str.size() - 1; ++k) {
				bytes.push_back(0);
			}

			return bytes;
		}
	};
	static base b58s(b58_ordered), b64s(b64_ordered);

#define DOMAIN_CHECK(c) ('0'<=(c)&&(c)<='9'||'a'<=(c)&&(c)<='f'||'A'<=(c)&&(c)<='F')

	const char* BASE58TABLE = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";


	class bignum_c
	{
	public:
		BIGNUM* _p = nullptr;
	public:
		bignum_c()
		{
			_p = BN_new();
		}

		bignum_c(BIGNUM* p) :_p(p)
		{
		}

		~bignum_c()
		{
			if (_p)
			{
				BN_free(_p);
			}
		}
		operator BIGNUM* () { return _p; }
		BIGNUM** get_ptr() { return &_p; }
	public:
		int num_bytes() {
			return ((BN_num_bits(_p) + 7) / 8);
			//return BN_num_bytes(_p);
		}
		std::string bn2hex()
		{
			auto s = BN_bn2hex(_p);
			std::string r;
			if (s)
			{
				r = s;
				OPENSSL_free(s);
			}
			return r;
		}
		std::string bn2dec()
		{
			auto s = BN_bn2dec(_p);
			std::string r;
			if (s)
			{
				r = s;
				OPENSSL_free(s);
			}
			return r;
		}
		std::string bn2bin()
		{
			std::string r;
			r.resize(num_bytes());
			auto s = BN_bn2bin(_p, (unsigned char*)r.data());
			if (s != r.size())
			{
				r.resize(s);
			}
			return r;
		}
		bool bin2bn(const char* d, int len)
		{

			auto s = BN_bin2bn((unsigned char*)d, len, _p);
			if (!_p)
			{
				_p = s;
			}
			return s;
		}
		int hex2bn(const char* str)
		{
			return BN_hex2bn(&_p, str);
		}
		int dec2bn(const char* str)
		{
			return BN_dec2bn(&_p, str);
		}
		int asc2bn(const char* str)
		{
			return BN_asc2bn(&_p, str);
		}
	private:

	};
	// Base58编码可以表示的比特位数为Log258 {\displaystyle \approx } \approx5.858bit。经过Base58编码的数据为原始的数据长度的1.37倍
	static unsigned char b58[256] = {
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 0, 1, 2, 3, 4, 5, 6,
		7, 8, 255, 255, 255, 255, 255, 255,
		255, 9, 10, 11, 12, 13, 14, 15,
		16, 255, 17, 18, 19, 20, 21, 255,
		22, 23, 24, 25, 26, 27, 28, 29,
		30, 31, 32, 255, 255, 255, 255, 255,
		255, 33, 34, 35, 36, 37, 38, 39,
		40, 41, 42, 43, 255, 44, 45, 46,
		47, 48, 49, 50, 51, 52, 53, 54,
		55, 56, 57, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255,
	};
	std::string base58decode(const std::string& b)
	{
		std::string result = "";
		BN_CTX* bnctx = BN_CTX_new();
		bignum_c answer;// : = big.NewInt(0);
		bignum_c j;// : = big.NewInt(1);

		BN_set_word(answer, 0);
		BN_set_word(j, 1);
		bignum_c scratch, t;
		for (int i = b.size() - 1; i >= 0; i--) {
			//字符，ascii码表的简版-->得到字符代表的值(0，1,2，..57)
			auto tmp = b58[b[i]];
			//出现不该出现的字符
			if (tmp == 255) {
				return "";
			}
			BN_set_word(scratch, tmp);
			//scratch = j*scratch
			BN_mul(t, j, scratch, bnctx);
			BN_copy(scratch, t);
			BN_add(t, answer, scratch);
			BN_copy(answer, t);
			//每次进位都要乘上58
			BN_mul_word(j, 58);
		}

		//得到大端的字节序
		auto tmpval = answer.bn2bin();

		int numZeros;
		for (numZeros = 0; numZeros < b.size(); numZeros++) {
			//得到高位0的位数
			if (b[numZeros] != b58idx0) {
				break;
			}
		}
		//得到原来数字的长度
		auto flen = numZeros + tmpval.size();
		if (numZeros > 0)
		{
			tmpval.insert(tmpval.begin(), (size_t)numZeros, 0);
		}
		BN_CTX_free(bnctx);
		return tmpval;
	}
	std::string base58encode(const std::string& str)
	{
		std::string result = "";
		BN_CTX* bnctx = BN_CTX_new();
		bignum_c bn, bn0, bn58, dv, rem;
		bn.bin2bn(str.c_str(), str.size());
		auto k = bn.bn2bin();
		//printf("bn:%s\n", BN_bn2dec(bn));
		bn58.hex2bn("3a");//58
		bn0.hex2bn("0");
		while (BN_cmp(bn, bn0) > 0) {
			BN_div(dv, rem, bn, bn58, bnctx);
			BN_copy(bn, dv);
			//printf("dv: %s\n", BN_bn2dec(dv));
			//printf("rem:%s\n", BN_bn2dec(rem));
			char base58char = BASE58TABLE[BN_get_word(rem)];
			result += base58char;
		}
		BN_CTX_free(bnctx);
		auto s = str.c_str();
		int64_t len = str.size();
		for (; len > 0 && *s == 0; s++, len--)
			continue;
		len = str.size() - len;
		if (len > 0)
		{
			result.append(len, b58idx0);
		}
		std::reverse(result.begin(), result.end());
		return result;
	}
	std::string ecc_c::b58_encode(const std::string& source)
	{
		return base58encode(source);
		return b58s.encode(source);
	}
	std::string ecc_c::b58_decode(const std::string& str)
	{
		return base58decode(str);
		return b58s.decode(str);
	}
	std::string ecc_c::b64_encode(const std::string& source)
	{
		return b64s.encode(source);
	}
	std::string ecc_c::b64_decode(const std::string& str)
	{
		return b64s.decode(str);
	}
#endif // nobase58

	ecc_c::ecc_c()
	{
	}
	ecc_c::ecc_c(int nid)
	{
		init(nid);
	}
	ecc_c::~ecc_c()
	{
		if (eckey)
			EC_KEY_free((EC_KEY*)eckey);
	}

	void ecc_c::swap(ecc_c& c)
	{
		std::swap(*this, c);
	}
	std::string ecc_c::get_pubkey1(bool isbstr)
	{
		std::string ret;
		auto point = (struct ec_point_st*)EC_KEY_get0_public_key((EC_KEY*)eckey);
		auto group = (struct ec_group_st*)EC_KEY_get0_group((EC_KEY*)eckey);
		unsigned char buf[1024] = {};
		auto ks = EC_GROUP_order_bits(group);
		auto plen = EC_POINT_point2oct(group, point, POINT_CONVERSION_COMPRESSED, buf, 1024, NULL);
		if (plen > 0)
		{
			if (isbstr)
				ret = ecc_c::bin2hex((char*)buf, plen, true);
			else
				ret.assign((char*)buf, plen);
		}
		else {
			erc = -1;
		}

		return ret;
	}
	std::string ecc_c::get_prikey1()
	{
		std::string ret;
		unsigned char buf[1024] = {};
		auto plen = EC_KEY_priv2oct((EC_KEY*)eckey, buf, 1024);
		if (plen > 0)
		{
			ret.assign((char*)buf, plen);
		}
		else {
			erc = -1;
		}
		return ret;
	}
	std::string ecc_c::compute_key(const std::string& pubkey2)
	{
		auto group = EC_KEY_get0_group((EC_KEY*)eckey);
		auto pointc = EC_POINT_new(group);
		EC_POINT_oct2point(group, pointc, (unsigned char*)pubkey2.data(), pubkey2.size(), NULL);
		_shared.resize(64);
		unsigned char* shared = (unsigned char*)_shared.data();
		size_t slen = 0;
		if (0 == (slen = ECDH_compute_key(shared, 64, pointc, (EC_KEY*)eckey, NULL))) erc = -3;
		_shared.resize(slen);
		EC_POINT_free(pointc); pointc = nullptr;
		return _shared;
	}
	std::string ecc_c::get_shared()
	{
		return _shared;
	}
	std::string ecc_c::get_pubkey()
	{
		if (_pubkey.empty())
		{
			unsigned char buf[1024] = {};
			unsigned char* opk = buf;
			auto pl = i2o_ECPublicKey((EC_KEY*)eckey, &opk);
			if (pl)
			{
				_pubkey.assign((char*)buf, pl);
			}
		}
		auto pk = get_pubkey1();
		return _pubkey;
	}
	std::string ecc_c::get_prikey()
	{
		if (_private_key.empty())
		{
			unsigned char buf[1024] = {};
			unsigned char* opk = buf;
			auto pl1 = i2d_ECPrivateKey((EC_KEY*)eckey, &opk);
			if (pl1 > 0)
				_private_key.assign((char*)buf, pl1);
		}
		return _private_key;
	}

	std::string get_parameters(EC_KEY* ek)
	{
		unsigned char buf[1024] = {};
		auto pb = buf;
		int n = i2d_ECParameters(ek, &pb);
		std::string ret;
		if (n)
		{
			ret.assign((char*)buf, n);
		}
		return ret;
	}
	// 保存蜜钥到json

	std::string ecc_c::save_key2json(const std::string& fn)
	{
		njson d;
		d["pri"] = b64_encode(get_prikey1());
		d["pub"] = b64_encode(get_pubkey1());
		d["parameters"] = b64_encode(get_parameters((EC_KEY*)eckey));
		d["nid"] = EC_GROUP_get_curve_name(EC_KEY_get0_group((EC_KEY*)eckey));
		auto ds = d.dump();
		if (fn.c_str() && fn.size())
			save_file(fn.c_str(), ds);
		return ds;
	}
	// 初始化新

	void ecc_c::init(int nid)
	{
		if (!(nid > 0))nid = NID_secp256k1;// NID_X9_62_prime256v1;
		if (eckey)EC_KEY_free((EC_KEY*)eckey);
		_nid = nid;
		eckey = EC_KEY_new_by_curve_name(nid);//NID_secp521r1
	}
	void ecc_c::init(const std::string& parameters)
	{
		if (eckey)EC_KEY_free((EC_KEY*)eckey);
		auto ps = (const unsigned char*)parameters.c_str();
		eckey = d2i_ECParameters(nullptr, &ps, parameters.size());
	}
	bool ecc_c::empty()
	{
		return !eckey;
	}
	void ecc_c::new_key(int nid)
	{
		if (!eckey)
		{
			init(nid > 0 ? nid : NID_secp256k1);
		}
		erc = EC_KEY_generate_key((EC_KEY*)eckey);
	}
	void ecc_c::new_key(const char* sn)
	{
		int nid = OBJ_sn2nid(sn);
		if (!eckey)
		{
			init(nid > 0 ? nid : NID_secp256k1);
		}
		erc = EC_KEY_generate_key((EC_KEY*)eckey);
	}

	// 从私钥数据初始化，公钥可为空自动创建

	void ecc_c::set_prikey(const std::string& prikey, const std::string& parameters, int nid)
	{
		if (prikey.size())
		{
			auto ek = (EC_KEY*)eckey;
			auto opk = (const unsigned char*)prikey.c_str();
			auto nek = d2i_ECPrivateKey(&ek, &opk, prikey.size());
			if (nek)
			{
				_private_key.clear();
				eckey = nek;
			}
			else
			{
				if (parameters.size())
				{
					init(parameters);
				}
				else
				{
					init(nid);
				}
				erc = EC_KEY_oct2priv((EC_KEY*)eckey, (unsigned char*)prikey.data(), prikey.size());
				if (erc == 1)
				{
					gen_pubkey();
				}
			}
		}
	}
	void ecc_c::set_pubkey(const std::string& pubkey, const std::string& parameters, int nid)
	{
		if (pubkey.size())
		{
			if (parameters.size())
			{
				init(parameters);
			}
			else
			{
				init(nid);
			}
			auto pubk = (const unsigned char*)pubkey.data();
			//EC_KEY* a = (EC_KEY*)eckey;
			//auto oi = o2i_ECPublicKey(&a, &pubk, pubkey.size());
			auto hr = EC_KEY_oct2key((EC_KEY*)eckey, pubk, pubkey.size(), nullptr);
			_pubkey.clear();
		}
	}
	int boms1(const char* str)
	{
		//UTF-8
		char utf8[] = { (char)0xEF ,(char)0xBB ,(char)0xBF };
		//UTF-16（大端序）
		char utf16d[] = { (char)0xFE ,(char)0xFF };
		//UTF-16（小端序）
		char utf16[] = { (char)0xFF ,(char)0xFE };
		//UTF-32（大端序）
		char utf32d[] = { (char)0x00 ,(char)0x00 ,(char)0xFE ,(char)0xFF };
		//UTF-32（小端序）
		char utf32[] = { (char)0xFF ,(char)0xFE ,(char)0x00 ,(char)0x00 };

		int bomlen[] = { 3,2,2,4,4 };
		char* bom[] = { utf8,utf16d,utf16,utf32d,utf32 };
		int u = -1;
		for (int i = 0; i < 5; ++i)
		{
			if (0 == memcmp(str, bom[i], bomlen[i]))
			{
				u = i;
			}
		}
		int ret = 0;
		if (u >= 0)
		{
			ret = bomlen[u];
		}
		return ret;
	}

	int save_file(const char* path, std::string& d)
	{
		FILE* fp;
		fp = fopen(path, "wb");
		if (fp)
		{
			fwrite(d.c_str(), 1, d.size(), fp);
			fclose(fp);
		}
		return d.size() && fp ? 0 : -1;
	}
	int read_file(const char* path, std::string& d)
	{
		long len;
		FILE* file = fopen(path, "rb");
		if (!file)
			return -1;
		fseek(file, 0, SEEK_END);
		len = ftell(file);
		rewind(file);
		d.resize(len);
		auto buffer = d.data();
		if (fread(buffer, 1, len, file) != (size_t)len)
		{
			fclose(file);
			d.clear();
			return -1;
		}
		fclose(file);
		return len;
	}
	inline bool isse0(int ch)
	{
		return ch == 0 || (ch > 0 && isspace(ch));
	}
	bool is_json1(const char* str, size_t len)
	{
		bool ret = false;
		char* te = (char*)str + len - 1;
		char* t = (char*)str;
		if (str && len > 1)
		{
			do
			{
				while (isse0(*t))
				{
					t++;
				}
				if (*t != '[' && *t != '{')
				{
					break;
				}
				while (isse0(*te))
				{
					te--;
				}
				if ((*t == '[' && *te == ']') || (*t == '{' && *te == '}'))
				{
					ret = true;
				}
			} while (0);
		}
		return ret;
	}
	njson fread_json(std::string fn)
	{
		std::string v;
		read_file(fn.c_str(), v);
		njson ret;
		if (v.size())
		{
			try
			{
				int bs = boms1(v.data());
				if (is_json1(v.data() + bs, v.size() - bs))
				{
					ret = njson::parse(v.begin() + bs, v.end());
				}
				else
				{
					ret = njson::from_cbor(v.begin() + bs, v.end());
				}
			}
			catch (const std::exception& e)
			{
				printf("parse json:\t%s\n", e.what());
			}
		}
		return ret;
	}
	std::string toStr1(njson& k) {
		std::string r;
		if (k.is_string())
		{
			r = k.get<std::string>();
		}
		return r;
	}
	int toInt1(njson& k) {
		int r = -1;
		if (k.is_number())
		{
			r = k.get<int>();
		}
		return r;
	}
	// 从私钥文件初始化

	void ecc_c::load_key2file(const std::string& fn)
	{
		auto d = fread_json(fn);
		if (d.size())
		{
			auto pri = b64_decode(toStr1(d["pri"]));
			auto pub = b64_decode(toStr1(d["pub"]));
			auto parameters = b64_decode(toStr1(d["parameters"]));
			int nid = toInt1(d["nid"]);
			if (pri.size())
			{
				set_prikey(pri, parameters, nid);
			}
			else if (pub.size())
			{
				set_pubkey(pub, parameters, nid);
			}
		}
	}

	// 创建公钥

	void ecc_c::gen_pubkey()
	{
		BN_CTX* ctx = NULL;
		auto pk = EC_KEY_get0_private_key((EC_KEY*)eckey);
		if (pk)
		{
			auto group = EC_KEY_get0_group((EC_KEY*)eckey);
			auto pub_key = EC_POINT_new(group);
			if (EC_POINT_mul(group, pub_key, pk, NULL, NULL, ctx))
			{
				EC_KEY_set_public_key((EC_KEY*)eckey, pub_key);
			}
			if (pub_key)
			{
				EC_POINT_free(pub_key);
			}
		}
	}
	class free_auto
	{
	public:
		free_auto()
		{
		}

		~free_auto()
		{
			for (auto p : t)
			{
				if (p)OPENSSL_free(p);
			}
		}
		void push(void* p)
		{
			t.insert(p);
		}
	private:
		std::set<void*> t;
	};

	bool ecc_c::pem_mem(const std::string& pemdata, const std::string& pass)
	{
		auto biop = BIO_new_mem_buf(pemdata.data(), pemdata.size());
		EC_KEY* (*pem_func[2])(BIO * bp, EC_KEY * *x, pem_password_cb * cb, void* u) = { PEM_read_bio_ECPrivateKey,  PEM_read_bio_EC_PUBKEY };
		EC_KEY* ek = 0;
		int idx = -1;
		bool ret = false;
		int pos = pemdata.find(PEM_STRING_ECPRIVATEKEY);
		if (pos > 0)
		{
			idx = 0;
		}
		else
		{
			pos = pemdata.find(PEM_STRING_PUBLIC);
			if (pos > 0)
			{
				idx = 1;
			}
		}
		if (!(idx < 0 || idx>1))
			ek = pem_func[idx](biop, 0, 0, (char*)pass.c_str());
		BIO_free(biop);
		if (ek)
		{
			if (eckey)EC_KEY_free((EC_KEY*)eckey);
			eckey = ek;
			ret = true;
			//get_pem(idx);
		}
		return ret;
	}
	int pass_de_cb(char* buf, int size, int rwflag)
	{
		std::string str;
		do
		{
			printf("输入私钥密码: ");
			for (;;)
			{
				char ch = getch();

				if (ch == '\r')
				{
					break;
				}
				if (ch == '\b')//遇到回车键时`在这里插入代码片`
				{
					if (str.size())
					{
						printf("\b \b");//对显示的字符进行回车修改，两个\b之间存在一个空格，我用vs2015进行的测试，不懂存不存在编译器的问题，有待考证
						str.pop_back();			//修改输入的字符
					}
					continue;
				}
				str.push_back(ch);
				printf("*");
			}
			if (str.size() > size)
				str.resize(size);
			if (str.size())
			{
				memcpy(buf, str.c_str(), str.size());
			}
		} while (str.empty());
		printf("\n");
		return str.size();
	}
	bool ecc_c::pem_mem_cb(const std::string& pemdata, std::function<int(char* buf, int size, int rwflag)> cb)
	{
		EC_KEY* (*pem_func[2])(BIO * bp, EC_KEY * *x, pem_password_cb * cb, void* u) = { PEM_read_bio_ECPrivateKey, PEM_read_bio_EC_PUBKEY };
		EC_KEY* ek = 0;
		int idx = -1;
		bool ret = false;
		int pos = pemdata.find(PEM_STRING_ECPRIVATEKEY);
		if (pos > 0)
		{
			idx = 0;
		}
		else
		{
			pos = pemdata.find(PEM_STRING_PUBLIC);
			if (pos > 0)
			{
				idx = 1;
			}
		}
		int inc = 3;
		if (!cb)
		{
			cb = pass_de_cb;
		}
		do
		{
			auto biop = BIO_new_mem_buf(pemdata.data(), pemdata.size());
			if (!(idx < 0 || idx>1))
				ek = pem_func[idx](biop, 0, pem_pass_cbs, &cb);
			BIO_free(biop);
			if (ek)
			{
				if (eckey)EC_KEY_free((EC_KEY*)eckey);
				eckey = ek;
				ret = true;
				break;
			}
		} while (0 < inc--);
		return ret;
	}
	int ecc_c::pem_pass_cbs(char* buf, int size, int rwflag, void* userdata)
	{
		auto& kcb = *((std::function<int(char* buf, int size, int rwflag)>*)userdata);
		return kcb(buf, size, rwflag);
	}


	void ciph_func(const EVP_CIPHER* ciph, const char* from, const char* to, void* x)
	{
		auto p = (std::map<std::string, const void*>*)x;
		if (ciph && p) {
			p->insert({ from, ciph });
		}
	}
	void md_func(const EVP_MD* ciph, const char* from, const char* to, void* x)
	{
		auto p = (std::map<std::string, const EVP_MD*>*)x;
		if (ciph && p) {
			p->insert({ from, ciph });
		}
	}
	// 获取加密算法列表
	void get_cipher(std::map<std::string, const void*>* m)
	{
		EVP_CIPHER_do_all(ciph_func, m);
	}
	std::map<std::string, const void*> ecc_c::get_ciphers()
	{
		std::map<std::string, const void*> m;
		EVP_CIPHER_do_all(ciph_func, &m);
		return m;
	}
	// 获取摘要算法列表
	std::map<std::string, const void*> ecc_c::get_mds()
	{
		//EVP_MD*
		std::map<std::string, const void*> m;
		EVP_MD_do_all(md_func, &m);
		return m;
	}
	/// 获取所有支持的曲线
	std::map<int, const char*> ecc_c::get_curve_names()
	{
		std::map<int, const char*> ret;
		auto n = EC_get_builtin_curves(0, 0);
		if (n > 0)
		{
			std::vector<EC_builtin_curve> r;
			r.resize(n);
			EC_get_builtin_curves(r.data(), n);
			for (auto& it : r)
			{
				ret[it.nid] =// it.comment;
					OBJ_nid2sn(it.nid);
			}
		}
		return ret;
	}
	/// 获取所有支持的曲线
	std::map<const char*, int> ecc_c::get_curve_names1()
	{
		std::map<const char*, int> ret;
		auto n = EC_get_builtin_curves(0, 0);
		if (n > 0)
		{
			std::vector<EC_builtin_curve> r;
			r.resize(n);
			EC_get_builtin_curves(r.data(), n);
			for (auto& it : r)
			{
				ret[OBJ_nid2sn(it.nid)] = it.nid;
			}
		}
		return ret;
	}
	std::string digest_cb(EVP_MD_CTX* c, EVP_MD* md, const unsigned char* salt, size_t saltlen, const unsigned char* data, size_t datal, int count, size_t out_size)
	{
		std::string ret;
		int addmd = 0;
		int64_t n = out_size;
		uint32_t mds = 0;
		unsigned char md_buf[EVP_MAX_MD_SIZE] = {};
		do
		{
			if (!EVP_DigestInit_ex(c, md, NULL))
				break;
			if (addmd++)
				if (!EVP_DigestUpdate(c, &(md_buf[0]), mds))
					break;
			if (!EVP_DigestUpdate(c, data, datal))
				break;
			if (salt && saltlen > 0)
				if (!EVP_DigestUpdate(c, salt, saltlen))
					break;
			if (!EVP_DigestFinal_ex(c, &(md_buf[0]), &mds))
				break;

			for (int i = 1; i < count; i++) {
				if (!EVP_DigestInit_ex(c, md, NULL))
					break;
				if (!EVP_DigestUpdate(c, &(md_buf[0]), mds))
					break;
				if (!EVP_DigestFinal_ex(c, &(md_buf[0]), &mds))
					break;
			}
			ret.append((char*)md_buf, mds);
			n -= mds;
		} while (n > 0);
		if (out_size > 0 && ret.size() > out_size)
			ret.resize(std::max(out_size, (size_t)mds));
		return ret;
	}
	std::string ecc_c::do_md(const void* vmd, const char* data, size_t len)
	{
		return do_dgst(vmd, data, len, "", 1, 0);
	}
	std::string ecc_c::do_md(const void* vmd, const std::string& data)
	{
		return do_dgst(vmd, data.data(), data.size(), "", 1, 0);
	}

	std::string ecc_c::do_dgst(const void* vmd, const char* data, size_t len, const std::string& salt, int count, int out_size)
	{
		std::string ret;
		auto ctx = EVP_MD_CTX_new();
		if (ctx)
		{
			auto md = (EVP_MD*)vmd;
			unsigned char d[EVP_MAX_MD_SIZE] = {};
			unsigned int d_len = 0;
			if (count < 1)count = 1;
			ret = digest_cb(ctx, md, (const unsigned char*)salt.data(), salt.size(), (const unsigned char*)data, len, count, out_size);
			EVP_MD_CTX_free(ctx);
		}
		return ret;
	}
	std::string ecc_c::crc32s(const void* data, size_t len)
	{
		char bufer[32] = { 0 };
		sprintf(bufer, "%8X", crc32(0, (Bytef*)data, len));
		return bufer;
	}
	uint32_t ecc_c::crc32u(const void* data, size_t len)
	{
		return crc32(0, (Bytef*)data, len);
	}


	std::string ecc_c::get_pem(int t, const std::string& kstr)
	{
		std::string ret;
		auto biop = BIO_new(BIO_s_mem());
		int er = -1;
		switch (t)
		{
		case 0:
		{
			auto cipher = EVP_aes_256_cbc();
			if (kstr.empty())cipher = nullptr;
			er = PEM_write_bio_ECPrivateKey(biop, (EC_KEY*)eckey, cipher, (unsigned char*)kstr.c_str(), kstr.size(), 0, 0);
		}
		break;
		case 1:
			er = PEM_write_bio_EC_PUBKEY(biop, (EC_KEY*)eckey);
			break;
		case 2:
			er = PEM_write_bio_ECPKParameters(biop, EC_KEY_get0_group((EC_KEY*)eckey));
			break;
		default:
			break;
		}

		int64_t dlen = BIO_number_written(biop);
		if (dlen > 0)
		{
			ret.resize(dlen);
			int kkk = BIO_read(biop, (void*)ret.data(), dlen);
		}
		BIO_free(biop);
		return ret;
	}
	void ecc_c::save_pem(std::string* _private, std::string* _pub, const std::string& pass)
	{
		if (_private)
		{
			//_pass_cb = pass_cb;
			auto d = get_pem(2, pass);
			d += get_pem(0, pass);
			_private->swap(d);
		}
		if (_pub)
		{
			auto d = get_pem(1, pass);
			_pub->swap(d);
		}
	}
	/*
		参数一般用md5或sha之类，
		返回签名二进制数据
	*/
	std::string ecc_c::do_sign(const std::string& dgst)
	{
		std::string ret;
		ECDSA_SIG* sig = ECDSA_do_sign((unsigned char*)dgst.data(), dgst.size(), (EC_KEY*)eckey);
		if (sig)
		{
			unsigned char sigbuf[1024] = {};
			unsigned char* sb = sigbuf;
			auto n = i2d_ECDSA_SIG(sig, &sb);// (unsigned char**)&sigbuf);
			if (n > 0)
				ret.assign((char*)sigbuf, n);
			ECDSA_SIG_free(sig);
		}
		return ret;
	}

	/*-
		dgst摘要
		sig签名
	* returns
	*      1: correct signature
	*      0: incorrect signature
	*     -1: error
	*/

	bool ecc_c::do_verify(const std::string& dgst, const std::string& sig)
	{
		int ret = ECDSA_verify(0, (unsigned char*)dgst.data(), dgst.size(), (unsigned char*)sig.data(), sig.size(), (EC_KEY*)eckey);
		return ret == 1;
	}
	// 加密解密
	bool hz::ecc_c::cipher_encrypt(const void* cipher, const void* data, int size, const std::string& key, std::string& outd, std::string& iv, int ps)
	{
		if (!data || !cipher || size < 0 || key.empty())return false;
		cipher_data_t ct = {};
		ct.data = (char*)data;
		ct.dsize = size;
		ct.kstr = (char*)key.c_str();
		ct.klen = key.size();
		ct.cipher = cipher;
		bool isu = (data == outd.data());
		outd.resize((size_t)size + 24 + max_iv_len);
		ct.outd = (char*)outd.data();
		if (isu)
		{
			ct.data = (char*)ct.outd;
		}

		if (iv.size() == max_iv_len)
		{
			memcpy(ct.iv, iv.data(), max_iv_len);
			if (ps >= max_iv_len)
			{
				memcpy(ct.outd, iv.data(), max_iv_len);
			}
			ct.ivlen = max_iv_len;
		}
		if (ps >= max_iv_len)
		{
			ct.outd = (char*)ct.outd + ps;
		}
		// 加密
		ct.enc = 1;
		int r = do_cipher(&ct);
		outd.resize(ct.out_size + ps);
		if (iv.empty())
		{
			iv.assign((char*)ct.iv, ct.ivlen);
			memcpy(outd.data(), (char*)ct.iv, ct.ivlen);
		}
		return r != 0;
	}
	bool hz::ecc_c::cipher_decrypt(const void* cipher, const void* data, int size, const std::string& key, std::string& outd, const std::string& iv)
	{
		if (!data || !cipher || size < 0 || key.empty())return false;
		cipher_data_t ct = {};
		ct.data = (char*)data;
		ct.dsize = size;
		ct.kstr = (char*)key.c_str();
		ct.klen = key.size();
		ct.cipher = cipher;
		bool isu = (data == outd.data());
		outd.resize((size_t)size + 24);
		ct.outd = (char*)outd.data();
		if (isu)
		{
			ct.data = ct.outd;
		}
		if (iv.size() == max_iv_len)
		{
			memcpy(ct.iv, iv.data(), max_iv_len);
			ct.ivlen = max_iv_len;
		}
		else
		{
			return false;
		}
		//解密
		ct.enc = 0;
		int r = do_cipher(&ct);
		outd.resize(ct.out_size);
		return r != 0;
	}

	bool ecc_c::encrypt(const void* ecp, const char* data, size_t size, const std::string& sharedkey, std::string& out)
	{
		assert(sharedkey.size() >= 48);
		std::string iv = sharedkey.substr(32, 16);
		std::string key = sharedkey.substr(0, 32);
		return ecc_c::cipher_encrypt(ecp, data, size, key, out, iv, 0);
	}
	bool ecc_c::decrypt(const void* ecp, const char* data, size_t size, const std::string& sharedkey, std::string& out)
	{
		assert(sharedkey.size() >= 48);
		std::string iv = sharedkey.substr(32, 16);
		std::string key = sharedkey.substr(0, 32);
		return ecc_c::cipher_decrypt(ecp, data, size, key, out, iv);
	}
	std::string ecc_c::randstr(int count)
	{
		std::string outs;
		if (count < 8)count = 8;
		outs.resize(count);
		auto nRet = RAND_bytes((unsigned char*)outs.data(), count);
		return outs;
	}
	bool ecc_c::encrypt_iv0(const void* ecp, const char* data, size_t size, const std::string& sharedkey, std::string& out)
	{
		assert(sharedkey.size() >= 6);
		std::string iv;		// 空就是用随机iv
		std::string key = sharedkey;
		int ts = max_iv_len;
		bool r = ecc_c::cipher_encrypt(ecp, data, size, key, out, iv, ts);
		return r;
	}
	bool ecc_c::decrypt_iv0(const void* ecp, const char* data, size_t size, const std::string& sharedkey, std::string& out)
	{
		std::string iv(data, max_iv_len);
		std::string key = sharedkey;
		return ecc_c::cipher_decrypt(ecp, data + max_iv_len, size - max_iv_len, key, out, iv);
	}


	// 计算摘要

	std::string ecc_c::sha256(const void* data, size_t len)
	{
		std::string ret;
		auto ctx = EVP_MD_CTX_new();
		if (ctx)
		{
			auto md = EVP_sha256();
			unsigned char d[EVP_MAX_MD_SIZE] = {};
			unsigned int d_len = 0;
			do
			{
				if (!EVP_DigestInit(ctx, md))
				{
					break;
				}
				if (!EVP_DigestUpdate(ctx, data, len))
				{
					break;
				}
				if (!EVP_DigestFinal(ctx, d, &d_len))
				{
					break;
				}
				ret.assign((char*)d, d_len);
			} while (0);
			EVP_MD_CTX_free(ctx);
		}
		return ret;
	}
	std::string c2md(const void* data, int len, const void* md0)
	{
		std::string ret;
		auto ctx = EVP_MD_CTX_new();
		unsigned char d[EVP_MAX_MD_SIZE] = {};
		unsigned int d_len = 0;
		auto md = (EVP_MD*)md0;
		if (md && ctx)
		{
			do
			{
				if (!EVP_DigestInit(ctx, md))
				{
					break;
				}
				if (!EVP_DigestUpdate(ctx, data, len))
				{
					break;
				}
				if (!EVP_DigestFinal(ctx, d, &d_len))
				{
					break;
				}
				ret.assign((char*)d, d_len);
			} while (0);
			EVP_MD_CTX_free(ctx);
		}
		return ret;
	}
	std::string ecc_c::pri_sign(const std::string& prikey, const std::string& dgst, const std::string& pass, std::string* pubkey)
	{
		ecc_c t;
		t.pem_mem(prikey, pass);
		if (pubkey)
		{
			*pubkey = t.get_pem(1, pass); //get_pubkey();
		}
		return t.do_sign(dgst);
	}
	bool ecc_c::pub_verify(const std::string& dgst, const std::string& sig, const std::string& pubkey)
	{
		ecc_c t;
		t.pem_mem(pubkey, "");
		EC_KEY* eckey = (EC_KEY*)t.eckey;
		int ret = ECDSA_verify(0, (unsigned char*)dgst.data(), dgst.size(), (unsigned char*)sig.data(), sig.size(), eckey);
		return ret == 1;
	}

	bool ecc_c::pub_verify1(const std::string& dgst, const std::string& sig, const std::string& pubkey, int nid)
	{
		if (!(nid > 0))nid = NID_secp256k1;// NID_X9_62_prime256v1;
		EC_KEY* eckey = EC_KEY_new_by_curve_name(nid);
		if (1 != EC_KEY_oct2key(eckey, (unsigned char*)pubkey.data(), pubkey.size(), nullptr))
			return false;
		int ret = ECDSA_verify(0, (unsigned char*)dgst.data(), dgst.size(), (unsigned char*)sig.data(), sig.size(), eckey);
		EC_KEY_free(eckey);
		return ret == 1;
	}
	int hz::ecc_c::pem_password_cbs(char* buf, int size, int rwflag, void* userdata)
	{
		auto p = (ecc_c*)userdata;
		int ret = 0;
		if (p && p->_pass_cb)
		{
			ret = p->_pass_cb(buf, size, rwflag);
		}
		return ret;
	}

	EC_KEY* pem2ptr(const std::string& pemdata, const std::string& pass)
	{
		auto biop = BIO_new_mem_buf(pemdata.data(), pemdata.size());
		EC_KEY* (*pem_func[2])(BIO * bp, EC_KEY * *x, pem_password_cb * cb, void* u) = { PEM_read_bio_ECPrivateKey,  PEM_read_bio_EC_PUBKEY };
		EC_KEY* ek = 0;
		int idx = -1;
		int pos = pemdata.find(PEM_STRING_ECPRIVATEKEY);
		if (pos > 0)
		{
			idx = 0;
		}
		else
		{
			pos = pemdata.find(PEM_STRING_PUBLIC);
			if (pos > 0)
			{
				idx = 1;
			}
		}
		if (!(idx < 0 || idx>1))
			ek = pem_func[idx](biop, 0, 0, (char*)pass.c_str());
		BIO_free(biop);
		return ek;
	}

	void* new_ecckey_nid(int nid)
	{
		EC_KEY* eckey = 0;
		if (!(nid > 0))nid = NID_secp256k1;
		eckey = EC_KEY_new_by_curve_name(nid);
		int erc = EC_KEY_generate_key((EC_KEY*)eckey);
		return eckey;
	}

	void* new_ecckey_pem(const char* pem, const char* pass)
	{
		EC_KEY* p = 0;
		if (pem && *pem)
			p = pem2ptr(pem, pass && *pass ? pass : "");
		return p;
	}

	std::string get_pem0(EC_KEY* eckey, int t, const std::string& kstr)
	{
		std::string ret;
		if (eckey)
		{
			auto biop = BIO_new(BIO_s_mem());
			int er = -1;
			switch (t)
			{
			case 0:
			{
				auto cipher = EVP_aes_256_cbc();
				if (kstr.empty())cipher = nullptr;
				er = PEM_write_bio_ECPrivateKey(biop, (EC_KEY*)eckey, cipher, (unsigned char*)kstr.c_str(), kstr.size(), 0, 0);
			}
			break;
			case 1:
				er = PEM_write_bio_EC_PUBKEY(biop, (EC_KEY*)eckey);
				break;
			case 2:
				er = PEM_write_bio_ECPKParameters(biop, EC_KEY_get0_group((EC_KEY*)eckey));
				break;
			default:
				break;
			}

			int64_t dlen = BIO_number_written(biop);
			if (dlen > 0)
			{
				ret.resize(dlen);
				int kkk = BIO_read(biop, (void*)ret.data(), dlen);
			}
			BIO_free(biop);
		}
		return ret;
	}
	std::string get_ecckey_pem(void* ecckey, const std::string& kstr)
	{
		return get_pem0((EC_KEY*)ecckey, 0, kstr);
	}

	std::string get_ecckey_pem_public(void* ecckey)
	{
		return get_pem0((EC_KEY*)ecckey, 1, "");
	}

	std::string get_compute_key(void* ecckey, void* pubkey)
	{
		int erc = 0;
		std::string _shared;
		auto pointc = (struct ec_point_st*)EC_KEY_get0_public_key((EC_KEY*)pubkey);
		//auto group = (struct ec_group_st*)EC_KEY_get0_group((EC_KEY*)pubkey); 
		_shared.resize(64);
		unsigned char* shared = (unsigned char*)_shared.data();
		size_t slen = 0;
		if (0 == (slen = ECDH_compute_key(shared, 64, pointc, (EC_KEY*)ecckey, NULL))) erc = -3;
		_shared.resize(slen);
		return _shared;
	}

	/*
		参数一般用md5或sha之类，
		返回签名二进制数据
	*/
	std::string ecprivatekey_sign(void* prikey, const std::string& dgst, std::string* pubkey)
	{
		std::string ret;
		ECDSA_SIG* sig = ECDSA_do_sign((unsigned char*)dgst.data(), dgst.size(), (EC_KEY*)prikey);
		if (sig)
		{
			unsigned char sigbuf[1024] = {};
			unsigned char* sb = sigbuf;
			auto n = i2d_ECDSA_SIG(sig, &sb);
			if (n > 0)
				ret.assign((char*)sigbuf, n);
			ECDSA_SIG_free(sig);
			if (pubkey)
			{
				*pubkey = get_pem0((EC_KEY*)prikey, 1, "");
			}
		}
		return ret;
	}

	bool public_verify(void* pubkey, const std::string& dgst, const std::string& sig)
	{
		EC_KEY* eckey = (EC_KEY*)pubkey;
		int ret = ECDSA_verify(0, (unsigned char*)dgst.data(), dgst.size(), (unsigned char*)sig.data(), sig.size(), eckey);
		return ret == 1;
	}






#if 1

	rsa_c::rsa_c()
	{
	}

	rsa_c::~rsa_c()
	{
		if (rkey)
			RSA_free((RSA*)rkey);
		rkey = 0;
	}
	std::string rsa_c::genrsa(int bits)
	{
		RSA* r = RSA_generate_key(bits, RSA_3, NULL, NULL);

		BIO* pri = BIO_new(BIO_s_mem());
		BIO* pub = BIO_new(BIO_s_mem());
		size_t pri_len;          // Length of private key
		size_t pub_len;          // Length of public key

		PEM_write_bio_RSAPrivateKey(pri, r, NULL, NULL, 0, NULL, NULL);
		PEM_write_bio_RSAPublicKey(pub, r);

		pri_len = (int)BIO_ctrl(pri, BIO_CTRL_PENDING, 0, 0);
		pub_len = (int)BIO_ctrl(pub, BIO_CTRL_PENDING, 0, 0);
		//pri_len = BIO_pending(pri);
		//pub_len = BIO_pending(pub);

		pri_key.resize(pri_len + 1);
		pub_key.resize(pub_len + 1);

		BIO_read(pri, pri_key.data(), pri_len);
		BIO_read(pub, pub_key.data(), pub_len);

		pri_key.resize(pri_len);
		pub_key.resize(pub_len);

		//printf("\n%s\n%s\n", pri_key, pub_key);
		if (rkey)
			RSA_free((RSA*)rkey);
		rkey = r;
		BIO_free_all(pub);
		BIO_free_all(pri);

		return pub_key;
	}
	void rsa_c::set_pubkey(const std::string& k)
	{
		if (rkey)
			RSA_free((RSA*)rkey);
		auto rk = (RSA*)rkey;
		BIO* pub = BIO_new(BIO_s_mem());
		BIO_write(pub, k.data(), k.size());
		PEM_read_bio_RSAPublicKey(pub, &rk, 0, 0);
		BIO_free_all(pub);
		rkey = rk;
	}
	std::string rsa_c::sign(const std::string& dgst)
	{
		int nid = NID_sha256;
		std::string sret;
		unsigned int siglen = dgst.size() * 3;
		if (siglen < 1024)
		{
			siglen = 1024;
		}
		sret.resize(siglen);
		int hr = RSA_sign(nid, (unsigned char*)dgst.c_str(), dgst.size(), (unsigned char*)sret.c_str(), &siglen, (RSA*)rkey);
		return sret.substr(0, siglen);
	}
	bool rsa_c::verify(const std::string& dgst, const std::string& sig)
	{
		int nid = NID_sha256;
		int ret = RSA_verify(nid, (unsigned char*)dgst.c_str(), dgst.size(), (unsigned char*)sig.c_str(), sig.size(), (RSA*)rkey);
		return ret;
	}
#endif // 1

	long biocb(BIO* b, int oper, const char* argp, int argi,
		long argl, long ret)
	{
		return ret;
	}
	void ecc_c::disp(const char* str, const void* pbuf, const int size)
	{
		int i = 0;
		if (str != NULL) {
			printf("%s:\n", str);
		}
		if (pbuf != NULL && size > 0) {
			for (i = 0; i < size; i++)
				printf("%02x ", *((unsigned char*)pbuf + i));
			putchar('\n');
		}
		putchar('\n');
	}
	std::string ecc_c::bin2hex(const char* str, int size, bool is_lower)
	{
		std::string ret;
		static const char* cts[] = { "0123456789ABCDEF", "0123456789abcdef" };
		auto ct = cts[is_lower ? 1 : 0];
		if (size < 0)
		{
			size = strlen(str);
		}
		if (size > 0 && str)
		{
			ret.reserve((size_t)size * 2);
			for (size_t i = 0; i < size; i++)
			{
				ret.push_back(ct[((unsigned char)str[i]) >> 4]);
				ret.push_back(ct[((unsigned char)str[i]) & 0x0f]);
			}
		}
		return ret;
	}
	std::string ecc_c::hex2bin(const char* str, int size)
	{
		std::string ret;
		if (size > 0 && str)
		{
			ret.reserve((size_t)size / 2);
			for (size_t i = 0; i < size; i++)
			{
				char c = str[i];
				char o;
				if (c >= '0' && c <= '9')
				{
					o = (c - '0') << 4;
				}
				else if (c >= 'a' && c <= 'f')
				{
					o = (c - 'a' + 10) << 4;
				}
				else if (c >= 'A' && c <= 'F')
				{
					o = (c - 'A' + 10) << 4;
				}
				else {
					continue;
				}
				c = str[++i];
				if (c >= '0' && c <= '9')
				{
					o |= (c - '0');
				}
				else if (c >= 'a' && c <= 'f')
				{
					o |= (c - 'a' + 10);
				}
				else if (c >= 'A' && c <= 'F')
				{
					o |= (c - 'A' + 10);
				}
				else {
					continue;
				}
				ret.push_back(o);
			}
		}
		return ret;
	}
	std::string ecc_c::ptr2hex(const void* p, bool is_lower)
	{
		char* t = (char*)&p;
		return bin2hex(t, sizeof(void*), is_lower);
	}
	std::string ecc_c::pem2pub(const std::string& pubstr)
	{
		std::string ret;
		ecc_c e;
		e.pem_mem(pubstr);
		ret = e.get_pubkey();
		return ret;
	}
	static unsigned int Decode85Byte(char c) { return c >= '\\' ? c - 36 : c - 35; }
	void decode85(const unsigned char* src, unsigned char* dst)
	{
		while (*src)
		{
			unsigned int tmp = Decode85Byte(src[0]) + 85 * (Decode85Byte(src[1]) + 85 * (Decode85Byte(src[2]) + 85 * (Decode85Byte(src[3]) + 85 * Decode85Byte(src[4]))));
			dst[0] = ((tmp >> 0) & 0xFF); dst[1] = ((tmp >> 8) & 0xFF); dst[2] = ((tmp >> 16) & 0xFF); dst[3] = ((tmp >> 24) & 0xFF);   // We can't assume little-endianness.
			src += 5;
			dst += 4;
		}
	}



#ifdef _WIN32

	std::vector<BYTE> MakePatternBytes(size_t a_Length)
	{
		std::vector<BYTE> result(a_Length);
		for (size_t i = 0; i < result.size(); i++)
		{
			result[i] = (BYTE)i;
		}

		return result;
	}

	std::vector<BYTE> MakeRandomBytes(size_t a_Length)
	{
		std::vector<BYTE> result(a_Length);
		for (size_t i = 0; i < result.size(); i++)
		{
			result[i] = (BYTE)rand();
		}

		return result;
	}
#endif // !_WIN32

}// !hz
struct ciphers_c
{
	std::map<std::string, const void*> _ciphers;
	std::vector<std::string> _ciphers_n;
};
void* ciphers_new_ctx() {
	auto p = new ciphers_c();
	if (p) {
		p->_ciphers = hz::ecc_c::get_ciphers();
		p->_ciphers_n.reserve(p->_ciphers.size());
		for (auto& it : p->_ciphers)
		{
			p->_ciphers_n.push_back(it.first);
		}
	}
	return p;
}
void ciphers_free_ctx(void* ctx) {
	if (ctx) {
		delete (ciphers_c*)ctx;
	}
}
// 获取支持的加密算法数量
int ciphers_get_count(void* ctx)
{
	auto p = (ciphers_c*)ctx;
	if (!p)return 0;
	auto& sctx = *p;
	if (sctx._ciphers.empty())
	{
		sctx._ciphers = hz::ecc_c::get_ciphers();
		sctx._ciphers_n.reserve(sctx._ciphers.size());
		for (auto& it : sctx._ciphers)
		{
			sctx._ciphers_n.push_back(it.first);
		}
	}
	return sctx._ciphers.size();
}
cipher_pt ciphers_get_idx(void* ctx, int idx)
{
	cipher_pt r = {};
	auto p = (ciphers_c*)ctx;
	if (!p)return r;
	auto& sctx = *p;
	auto n = ciphers_get_count(ctx);
	size_t x = idx;
	if (x < n)
	{
		auto& k = sctx._ciphers_n[x];
		auto& it = sctx._ciphers[k];
		r.n = k.c_str();
		r.ecp = it;
	}
	return r;
}
const char* ciphers_get_stri(void* ctx, int idx) {
	return ciphers_get_idx(ctx, idx).n;
}
cipher_pt ciphers_get_str(void* ctx, const char* str)
{
	cipher_pt r = {};
	auto p = (ciphers_c*)ctx;
	if (!p)return r;
	auto& sctx = *p;
	auto n = ciphers_get_count(ctx);
	if (str && *str > 0)
	{
		auto it = sctx._ciphers.find(str);
		if (it != sctx._ciphers.end())
		{
			r.n = it->first.c_str();
			r.ecp = it->second;
		}
	}
	return r;
}
void* ciphers_get_ptri(void* ctx, int idx)
{
	auto p = ciphers_get_idx(ctx, idx);
	return (void*)p.ecp;
}
void* ciphers_get_ptr(void* ctx, const char* str)
{
	auto p = ciphers_get_str(ctx, str);
	return (void*)p.ecp;
}
char* cpstr(const char* data, size_t n)
{
	auto cn = align_up(n + 1, 16);
	auto buf = (char*)malloc(cn);
	if (buf)
	{
		memcpy(buf, (char*)data, n);
		buf[n] = 0;
	}
	return buf;
}
//SSL_CBD(EVP_md5);
//SSL_CBD(EVP_sha256);
//SSL_CBD(EVP_aes_256_cbc);
// buf要至少32字节
char* ecc_sha256(const void* data, size_t n) {
	std::string b; 
	if (data && n > 0)
	{
		b = hz::ecc_c::sha256(data, n);
	} 
	return cpstr(b.data(), b.size());
}
char* ecc_md5(const void* data, size_t n) {
	std::string b; 
	if (data && n > 0)
	{
		auto md = EVP_md5();
		b = hz::c2md(data, n, md);
	} 
	return cpstr(b.data(), b.size());
}

int ecc_sha256_buf(const void* data, size_t n, char* buf) {
	std::string b;
	if (data && n > 0)
	{
		b = hz::ecc_c::sha256(data, n);
	}
	if (buf && b.size())
	{
		memcpy(buf, b.data(), b.size());
	}
	return b.size();
}
int ecc_md5_buf(const void* data, size_t n, char* buf) {
	std::string b;
	if (data && n > 0)
	{
		auto md = EVP_md5();
		b = hz::c2md(data, n, md);
	}
	if (buf && b.size())
	{
		memcpy(buf, b.data(), b.size());
	}
	return b.size();
}

uint32_t ecc_crc32u(const void* data, size_t len)
{
	return data && len > 0 ? hz::ecc_c::crc32u(data, len) : 0;
}

// 使用创建随机iv，密钥至少32字节，64字节
data_pt* encrypt_iv1(const void* ecp, const char* data, size_t size, const char* sharedkey, int ksize) {
	std::string b, key;
	int ns = sizeof(data_pt);
	data_pt* r = 0;
	if (!ecp)
	{
		ecp = EVP_aes_256_cbc();
	}
	if (sharedkey)
	{
		if (ksize < 0)
			key.assign(sharedkey);
		else
			key.assign(sharedkey, ksize);
		if (hz::ecc_c::encrypt_iv0(ecp, data, size, sharedkey, b))
		{
			ns += b.size();
			if (b.size())
			{
				ns = align_up(ns + 1, 16);
				auto p = (char*)malloc(ns);
				r = (data_pt*)p;
				r->size = b.size();
				r->data = p + sizeof(data_pt);
				r->data[r->size] = 0;
				memcpy(r->data, b.data(), r->size);
			}
		}
	}
	return r;
}
data_pt* decrypt_iv1(const void* ecp, const char* data, size_t size, const char* sharedkey, int ksize) {
	std::string b, key;
	int ns = sizeof(data_pt);
	data_pt* r = 0;
	if (!ecp)
	{
		ecp = EVP_aes_256_cbc();
	}
	if (sharedkey)
	{
		if (ksize < 0)
			key.assign(sharedkey);
		else
			key.assign(sharedkey, ksize);
		if (hz::ecc_c::decrypt_iv0(ecp, data, size, sharedkey, b))
		{
			ns += b.size();
			if (b.size())
			{
				ns = align_up(ns + 1, 16);
				auto p = (char*)malloc(ns);
				r = (data_pt*)p;
				r->size = b.size();
				r->data = p + sizeof(data_pt);
				r->data[r->size] = 0;
				memcpy(r->data, b.data(), r->size);
			}
		}
	}
	return r;
}
data_pt* decrypt_iv00(const void* ecp, const char* data, size_t size, const char* sharedkey, int ksize)
{
	std::string b;
	int ns = sizeof(data_pt);
	data_pt* r = 0;
	std::string iv;
	std::string key;
	if (!ecp)
	{
		ecp = EVP_aes_256_cbc();
	}
	if (sharedkey)
	{
		if (ksize < 0)
		{
			auto n = strlen(sharedkey);
			for (size_t i = 0; i < n; i++)
			{
				key.push_back(sharedkey[i]);
			}
		}
		iv.resize(16);
		if (ksize == 16) {

			char buffer[32] = { 0 };
			for (size_t i = 0; i < 16; i++)
			{
				sprintf(buffer, "%2X", sharedkey[i]);
				key.append(buffer);
			}
		}
		else if (ksize == 32) {
			for (size_t i = 0; i < 32; i++)
			{
				key.push_back(sharedkey[i]);
			}
		}
		else if (ksize >= 48) {
			for (size_t i = 0; i < 32; i++)
			{
				key.push_back(sharedkey[i]);
			}
			for (size_t i = 32; i < 48; i++)
			{
				iv[i] = (sharedkey[i]);
			}
		}
		if (hz::ecc_c::cipher_decrypt(ecp, data, size, key, b, iv))
		{
			ns += b.size();
			if (b.size())
			{
				ns = align_up(ns + 1, 16);
				auto p = (char*)malloc(ns);
				r = (data_pt*)p;
				r->size = b.size();
				r->data = p + sizeof(data_pt);
				memcpy(r->data, b.data(), r->size);
			}
		}
	}
	return r;
}
// 释放内存
void ecc_free(const void* p)
{
	if (p)
	{
		free((void*)p);
	}
}
data_pt* easy_en(const void* ecp, const void* data, size_t len, const char* keystr, int keylen)
{
	int ret = 0;
	if (!keystr)return 0;
	std::string key;
	keylen = keylen < 1 ? strlen(keystr) : keylen;
	if (keylen != 32)
	{
		key = hz::ecc_c::sha256(keystr, keylen);
		while (key.size() < EVP_MAX_KEY_LENGTH) {
			key += hz::ecc_c::sha256(key.c_str(), key.size());
		}
	}
	else {
		key.resize(keylen);
		memcpy(key.data(), keystr, keylen);
	}
	if (!ecp)
	{
		ecp = EVP_aes_256_cbc();
	}
	return encrypt_iv1(ecp, (char*)data, len, key.c_str(), key.size());
}
data_pt* easy_de(const void* ecp, const void* data, size_t len, const char* keystr, int keylen)
{
	int ret = 0;
	if (!keystr)return 0;
	std::string key;
	keylen = keylen < 1 ? strlen(keystr) : keylen;
	if (keylen != 32)
	{
		key = hz::ecc_c::sha256(keystr, keylen);
		while (key.size() < EVP_MAX_KEY_LENGTH) {
			key += hz::ecc_c::sha256(key.c_str(), key.size());
		}
	}
	else {
		key.resize(keylen);
		memcpy(key.data(), keystr, keylen);
	}
	if (!ecp)
	{
		ecp = EVP_aes_256_cbc();
	}
	return decrypt_iv1(ecp, (char*)data, len, key.c_str(), key.size());
}
size_t ecc_get_curve_count() 
{
	return EC_get_builtin_curves(0, 0);
}
// ecc非对称加密。需要用ecc_free释放内存
ecc_curve_t* ecc_get_curve_names()
{
	auto n = EC_get_builtin_curves(0, 0);
	ecc_curve_t* ret = 0;
	if (n > 0)
	{
		std::vector<EC_builtin_curve> r;
		r.resize(n);
		EC_get_builtin_curves(r.data(), n);
		ret = (ecc_curve_t*)malloc(sizeof(ecc_curve_t) * n);
		int i = 0;
		for (auto& it : r)
		{
			auto& kt = ret[i];
			kt.name = OBJ_nid2sn(it.nid); kt.nid = it.nid;
			i++;
		}
	}
	return ret;
}
// 用nid创建一个新的ecckey 
void* ecc_new_key_nid(int nid)
{
	EC_KEY* eckey = 0;
	if (!(nid > 0))nid = NID_secp256k1;
	eckey = EC_KEY_new_by_curve_name(nid);
	int erc = EC_KEY_generate_key((EC_KEY*)eckey);
	return eckey;
}

// 用于创建私钥或公钥，从pem数据创建ecckey。支持从get_ecckey_pem获取的数据创建。私钥pem或公钥pem数据 。ecc_get_pem设置密码时必需输入正确密码
void* ecc_new_key_pem(const char* pem, const char* pass)
{
	EC_KEY* p = 0;
	if (pem && *pem)
		p = hz::pem2ptr(pem, pass && *pass ? pass : "");
	return p;
}
//释放ecc key ptr
void ecc_free_key(void* ecckey)
{
	if (ecckey)
		EC_KEY_free((EC_KEY*)ecckey);
}

// 获取pem数据方便保存。可以设置密码
char* ecc_get_pem(void* prikey, const char* pass)
{
	auto str = hz::get_pem0((EC_KEY*)prikey, 0, pass && *pass ? pass : "");
	return cpstr(str.c_str(), str.size());
}
// 获取公钥，用于密钥交换
char* ecc_get_pem_public(void* prikey)
{
	auto str = hz::get_pem0((EC_KEY*)prikey, 1, "");
	return cpstr(str.c_str(), str.size());
}
// 获取共享密钥。输入我方的私钥，对方的公钥，参数使用new_ecckey_pem返回的指针，私钥也可以用new_ecckey_nid返回的指针
char* ecc_get_compute_key(void* prikey, void* pubkey)
{
	int erc = 0;
	std::string _shared;
	auto pointc = (struct ec_point_st*)EC_KEY_get0_public_key((EC_KEY*)pubkey);
	//auto group = (struct ec_group_st*)EC_KEY_get0_group((EC_KEY*)pubkey); 
	_shared.resize(64);
	unsigned char* shared = (unsigned char*)_shared.data();
	size_t slen = 0;
	if (0 == (slen = ECDH_compute_key(shared, 64, pointc, (EC_KEY*)prikey, NULL))) erc = -3;
	_shared.resize(slen);
	return cpstr(_shared.c_str(), slen);
}

// 私钥签名
char* ecc_privatekey_sign(void* prikey, const char* dgst, size_t dgst_size)
{
	char* buf = 0;
	ECDSA_SIG* sig = ECDSA_do_sign((unsigned char*)dgst, dgst_size, (EC_KEY*)prikey);
	if (sig)
	{
		unsigned char sigbuf[1024] = {};
		unsigned char* sb = sigbuf;
		auto n = i2d_ECDSA_SIG(sig, &sb);
		if (n > 0)
		{
			buf = cpstr((char*)sigbuf, n);
		}
		ECDSA_SIG_free(sig);
	}
	return buf;
}
// 用公钥验证： 返回1成功，其他值失败
int ecc_public_verify(void* pubkey, const char* dgst, size_t dgst_size, const char* sig, size_t sig_size)
{
	EC_KEY* eckey = (EC_KEY*)pubkey;
	int ret = ECDSA_verify(0, (unsigned char*)dgst, dgst_size, (unsigned char*)sig, sig_size, eckey);
	return ret;
}
#if 0

std::vector<char> dpt2v(data_pt* p) {
	std::vector<char> v;
	if (p && p->size && p->data)
	{
		v.resize(p->size);
		memcpy(v.data(), p->data, p->size);
	}
	return v;
}

int main()
{
	void* ctx = ciphers_new_ctx();
	// 获取支持的加密算法数量
	int n = ciphers_get_count(ctx);
	cipher_pt kecp128 = ciphers_get_str(ctx, "aes-128-cbc");
	cipher_pt kecp256 = ciphers_get_str(ctx, "aes-256-cbc");
	for (size_t i = 0; i < n; i++)
	{
		auto cp = ciphers_get_idx(ctx, i);
		printf("%s\n", cp.n);
	}
	ciphers_free_ctx(ctx);
	std::string str = "abc12345646523";
	std::string key = "hgfhfy1564512qqqqq";
	data_pt* ot = easy_en(kecp256.ecp, str.c_str(), str.size(), key.c_str(), key.size());
	data_pt* otd = easy_de(kecp256.ecp, ot->data, ot->size, key.c_str(), key.size());
	data_pt* ot1 = easy_en(0, str.c_str(), str.size(), key.c_str(), key.size());
	data_pt* otd1 = easy_de(0, ot1->data, ot1->size, key.c_str(), key.size());

	data_pt* ot0 = easy_en(kecp128.ecp, str.c_str(), str.size(), key.c_str(), key.size());
	data_pt* otd0 = easy_de(kecp128.ecp, ot0->data, ot0->size, key.c_str(), key.size());

	auto otv = dpt2v(ot);
	auto otv1 = dpt2v(ot1);
	auto otv0 = dpt2v(ot0);


	// 用nid创建一个新的ecckey 
	void* mekey = hz::new_ecckey_nid(0);
	void* mekey1 = hz::new_ecckey_nid(0);
	// 从pem数据创建ecckey。支持从get_ecckey_pem获取的数据创建。私钥pem或公钥pem数据 。密码可选
	//void* new_ecckey_pem(const char* pem, const char* pass);
	// 获取pem数据方便保存。
	std::string mepem = hz::get_ecckey_pem(mekey, "");
	std::string mepem_key = hz::get_ecckey_pem(mekey, "abc");// 私钥加密码
	std::string mepem1 = hz::get_ecckey_pem(mekey1, "");
	// 获取公钥
	std::string mekey_pub = hz::get_ecckey_pem_public(mekey);
	std::string mekey_pub1 = hz::get_ecckey_pem_public(mekey1);

	void* tkey0 = hz::new_ecckey_pem(mepem.c_str(), "");
	void* tkey = hz::new_ecckey_pem(mepem_key.c_str(), "abc");
	void* tkey1 = hz::new_ecckey_pem(mepem_key.c_str(), "");  //密码错误则创建失败

	void* pubkey = hz::new_ecckey_pem(mekey_pub.c_str(), "");
	void* pubkey1 = hz::new_ecckey_pem(mekey_pub1.c_str(), "");

	// 获取共享密钥。输入我方的私钥，对方的公钥，参数使用new_ecckey_pem返回的指针，私钥也可以用new_ecckey_nid返回的指针
	std::string k0 = hz::get_compute_key(mekey, pubkey1);
	std::string k1 = hz::get_compute_key(mekey1, pubkey);
	// 可以看出用对方公钥和自己私钥创建的共享密钥是一样的，可直接用来当aes密码
	std::string k[] = { hz::ecc_c::bin2hex(k0.c_str(),-1,true),hz::ecc_c::bin2hex(k1.c_str(),-1,true) };
	std::string dgst = hz::ecc_c::sha256(str.c_str(), str.size());
	// 私钥签名
	std::string pubkey0;
	std::string sign = hz::ecprivatekey_sign(mekey, dgst, &pubkey0);
	// 用公钥验证
	bool hr = hz::public_verify(pubkey, dgst, sign);

	// 私钥1签名。
	std::string sign1 = hz::ecprivatekey_sign(mekey1, dgst, 0);
	// 用公钥1验证
	bool hr1 = hz::public_verify(pubkey1, dgst, sign1);


	free_dt(ot);
	free_dt(ot1);
	free_dt(ot0);
	free_dt(otd);
	free_dt(otd1);
	free_dt(otd0);
	return 0;
}

#endif // 0
