#pragma once
#ifndef ecc_sv_h
#define ecc_sv_h
#endif // !ecc_sv_h

//#include <shared.h>

#ifdef  __cplusplus
extern "C" {
#endif
#ifndef EVP_MAX_IV_LENGTH
#define max_iv_len 16
#else
#define max_iv_len EVP_MAX_IV_LENGTH
#endif // !EVP_MAX_IV_LENGTH
#ifndef _ES_
#define _ES_
	typedef struct cipher_pt {
		const char* n;
		const void* ecp;
	}cipher_pt;
	typedef struct data_pt {
		size_t size;
		char* data;
	}data_pt;
#endif
	struct cipher_data_t {
		// *加密解密的数据
		void* data;
		// *长度
		int dsize;
		// *密钥则klen=64,密码则klen设置0
		char* kstr;
		int klen;
		// 可选输出缓存区，大小比输入+24
		void* outd = nullptr;
		// *加密算法
		const void* cipher;
		//const EVP_CIPHER* cipher;
		// *enc=0解密，1加密
		int enc = 1;
		/*
			加密随机生成iv，解密需要设置iv和长度
		*/
		unsigned char iv[max_iv_len] = {};
		int ivlen = 0;
		// 返回加密长度
		int out_size = 0;
		bool rawkey = false;
	};

	// return < 0失败
	int do_cipher(cipher_data_t* ct);

	//#define EXPORT_SV MCPP_EXPORT
#ifndef EXPORT_SV
#define EXPORT_SV 
#endif
	EXPORT_SV void* new_ciphers_ctx();
	// 获取支持的加密算法数量
	EXPORT_SV int get_ciphers_count(void* ctx);
	EXPORT_SV cipher_pt get_ciphers_idx(void* ctx, int idx);
	EXPORT_SV cipher_pt get_ciphers_str(void* ctx, const char* str);
	EXPORT_SV void* get_ciphers_ptri(void* ctx, int idx);
	EXPORT_SV void* get_ciphers_ptr(void* ctx, const char* str);
	// buf要至少32字节
	EXPORT_SV int ext_sha256(const void* data, int64_t n, char* buf);
	EXPORT_SV int ext_md5(const void* data, int64_t n, char* buf);
	EXPORT_SV uint32_t ext_crc32u(const void* data, int len);
	// 使用创建随机iv，密钥至少32字节，64字节
	EXPORT_SV data_pt* encrypt_iv1(const void* ecp, const char* data, size_t size, const char* sharedkey, int ksize);
	EXPORT_SV data_pt* decrypt_iv1(const void* ecp, const char* data, size_t size, const char* sharedkey, int ksize);
	EXPORT_SV data_pt* decrypt_iv00(const void* ecp, const char* data, size_t size, const char* sharedkey, int ksize);
	// 释放加密解密的内存
	EXPORT_SV void free_dt(data_pt* p);
	// 密码如果是32字节则不计算sha256
	EXPORT_SV data_pt* easy_en(const void* ecp, const void* data, size_t len, const char* keystr, int keylen);
	EXPORT_SV data_pt* easy_de(const void* ecp, const void* data, size_t len, const char* keystr, int keylen);

#ifdef  __cplusplus
}

#include <vector>
#include <string>
#include <map>
#include <functional>

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
		static std::string sha256(const void* data, int len);
		static std::string do_dgst(const void* vmd, const char* data, int len, const std::string& salt = "", int count = 1, int out_size = 0);
		static std::string do_md(const void* vmd, const std::string& data);
		static std::string do_md(const void* vmd, const char* data, int len);
		static std::string crc32s(const void* data, int len);
		static uint32_t crc32u(const void* data, int len);
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

#endif
//!cpp
