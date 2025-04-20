#pragma once
#ifndef ecc_sv_h
#define ecc_sv_h
#endif // !ecc_sv_h
 
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

	typedef struct ecc_curve_t {
		size_t nid;
		const char* name;
	}ecc_curve_t;

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


#ifdef  __cplusplus
} 
#endif
//!cpp

#ifndef EXPORT_SV
#define EXPORT_SV MC_EXPORT
#endif
EXPORT_SV void* ciphers_new_ctx();
// 下面获取到指定ecp后，如果不调用ctx参数的函数可以释放ctx
EXPORT_SV void ciphers_free_ctx(void* ctx);
// 获取支持的加密算法数量
EXPORT_SV int ciphers_get_count(void* ctx);
// 返回结构体，名称、ecp指针
EXPORT_SV cipher_pt ciphers_get_idx(void* ctx, int idx);
EXPORT_SV cipher_pt ciphers_get_str(void* ctx, const char* str);
EXPORT_SV void* ciphers_get_ptri(void* ctx, int idx);
// 返回字符串指针，失败返回0。不需要释放，free_ctx时会自动释放
EXPORT_SV const char* ciphers_get_stri(void* ctx, int idx);
// 从字符串查找加密算法ecp指针
EXPORT_SV void* ciphers_get_ptr(void* ctx, const char* str);

// 返回char*需要用ecc_free释放
EXPORT_SV void ecc_free(const void* p);
// sha256返回32字节，md5返回16字节
EXPORT_SV char* ecc_sha256(const void* data, size_t n);
EXPORT_SV char* ecc_md5(const void* data, size_t n);
EXPORT_SV int ecc_sha256_buf(const void* data, size_t n, char* buf);
EXPORT_SV int ecc_md5_buf(const void* data, size_t n, char* buf);
EXPORT_SV uint32_t ecc_crc32u(const void* data, size_t len);

// ecp为0则默认使用 aes_256_cbc加密 
// 密码如果是32字节则不计算sha256。需要用ecc_free释放内存
EXPORT_SV data_pt* easy_en(const void* ecp, const void* data, size_t len, const char* keystr, int keylen);
EXPORT_SV data_pt* easy_de(const void* ecp, const void* data, size_t len, const char* keystr, int keylen);

// ecc非对称加密。获取曲线返回指针需要用ecc_free释放内存 ecc_free(（ecc_curve_t*）p);
EXPORT_SV ecc_curve_t* ecc_get_curve_names();
// 获取曲线数量
EXPORT_SV size_t ecc_get_curve_count();
// 用nid创建一个新的ecckey 
EXPORT_SV void* ecc_new_key_nid(int nid);
// 用于创建私钥或公钥，从pem数据创建ecckey。支持从get_ecckey_pem获取的数据创建。私钥pem或公钥pem数据 。ecc_get_pem设置密码时必需输入正确密码
EXPORT_SV void* ecc_new_key_pem(const char* pem, const char* pass);
//释放ecc key ptr
EXPORT_SV void ecc_free_key(void* ecckey);

// 获取pem数据方便保存。可以设置密码
EXPORT_SV char* ecc_get_pem(void* prikey, const char* pass);
// 获取公钥，用于密钥交换
EXPORT_SV char* ecc_get_pem_public(void* prikey);
// 获取共享密钥。输入我方的私钥，对方的公钥，参数使用new_ecckey_pem返回的指针，私钥也可以用new_ecckey_nid返回的指针
EXPORT_SV char* ecc_get_compute_key(void* prikey, void* pubkey);
// 私钥签名
EXPORT_SV char* ecc_privatekey_sign(void* prikey, const char* dgst, size_t dgst_size);
// 用公钥验证： 返回1成功，其他值失败
EXPORT_SV int ecc_public_verify(void* pubkey, const char* dgst, size_t dgst_size, const char* sig, size_t sig_size);
