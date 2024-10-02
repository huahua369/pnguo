

#include <pch1.h>
#include <vkgui/mapView.h>
#include "mshell.h"
#if 1
#if 0

#define HAVE_INTTYPES_H
#define HAVE_STDLIB_H
/* #undef HAVE_SYS_SELECT_H */
/* #undef HAVE_SYS_SOCKET_H */
/* #undef HAVE_SYS_TIME_H */
/* #undef HAVE_ARPA_INET_H */
/* #undef HAVE_NETINET_IN_H */
#define HAVE_WINSOCK2_H

/* Functions */
/* #undef HAVE_STRCASECMP */
#define HAVE__STRICMP
#define HAVE_SNPRINTF
#define HAVE__SNPRINTF

/* Workaround for platforms without POSIX strcasecmp (e.g. Windows) */
#ifndef HAVE_STRCASECMP
# ifdef HAVE__STRICMP
# define strcasecmp _stricmp
# define HAVE_STRCASECMP
# endif
#endif

/* Symbols */
#define HAVE___FUNC__
#define HAVE___FUNCTION__

/* Workaround for platforms without C90 __func__ */
#ifndef HAVE___FUNC__
# ifdef HAVE___FUNCTION__
# define __func__ __FUNCTION__
# define HAVE___FUNC__
# endif
#endif
#endif
//#include "2180/libssh2.h" sftp已经include了
#include <libssh2_sftp.h>
#endif
/*
颜色值x  0   1   2   3   4   5   6   7
颜色     黑  红  绿  黄  蓝  紫  青  白
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>  
#include <stdio.h>
#include <ctype.h>
#include <inttypes.h>
#include <cassert> 
#ifndef _WIN32
#include <algorithm>
#include <pwd.h>
#include <grp.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <termios.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include <conio.h>
#include <sys/timeb.h>
#define strerror_r(e,buf,len) strerror_s(buf,len,e)
#endif // _WIN32
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#define INET6_ADDRSTRLEN 46
#endif // !INET_ADDRSTRLEN

#ifndef __S_IFMT
#define	__S_IFMT	0170000	/* These bits determine file type.  */

/* File types.  */
#define	__S_IFDIR	0040000	/* Directory.  */
#define	__S_IFCHR	0020000	/* Character device.  */
#define	__S_IFBLK	0060000	/* Block device.  */
#define	__S_IFREG	0100000	/* Regular file.  */
#define	__S_IFIFO	0010000	/* FIFO.  */
#define	__S_IFLNK	0120000	/* Symbolic link.  */
#define	__S_IFSOCK	0140000	/* Socket.  */

#endif
#ifndef S_ISDIR
#define	__S_ISTYPE(mode, mask)	(((mode) & __S_IFMT) == (mask))

#define	S_ISDIR(mode)	 __S_ISTYPE((mode), __S_IFDIR)
#define	S_ISCHR(mode)	 __S_ISTYPE((mode), __S_IFCHR)
#define	S_ISBLK(mode)	 __S_ISTYPE((mode), __S_IFBLK)
#define	S_ISREG(mode)	 __S_ISTYPE((mode), __S_IFREG)
#ifdef __S_IFIFO
# define S_ISFIFO(mode)	 __S_ISTYPE((mode), __S_IFIFO)
#endif
#ifdef __S_IFLNK
# define S_ISLNK(mode)	 __S_ISTYPE((mode), __S_IFLNK)
#endif
#endif //!S_ISDIR



#ifdef _WIN32
#define fseeki64 _fseeki64
#define ftelli64 _ftelli64
#else			
#define fseeki64 fseeko64
#define ftelli64 ftello64
#endif // _WIN32


#if 1
// 终端控制
// 清除屏幕

#define CLEAR() printf("\033[2J")

// 上移光标

#define MOVEUP(x) printf("\033[%dA", (x))

// 下移光标

#define MOVEDOWN(x) printf("\033[%dB", (x))

// 左移光标

#define MOVELEFT(y) printf("\033[%dD", (y))

// 右移光标

#define MOVERIGHT(y) printf("\033[%dC",(y))

// 定位光标

#define MOVETO(x,y) printf("\033[%d;%dH", (x), (y))

// 光标复位

#define RESET_CURSOR() printf("\033[H")

// 隐藏光标

#define HIDE_CURSOR() printf("\033[?25l")

// 显示光标

#define SHOW_CURSOR() printf("\033[?25h")

//反显

#define HIGHT_LIGHT() printf("\033[7m")

#define UN_HIGHT_LIGHT() printf("\033[27m")
#endif // 1

#ifndef _PWD_H
typedef unsigned int __uid_t;
typedef unsigned int __gid_t;
struct passwd
{
	char* pw_name; /* Username, POSIX.1 */
	char* pw_passwd; /* Password */
	__uid_t pw_uid; /*__uid_t User ID, POSIX.1 */
	__gid_t pw_gid; /*__gid_t Group ID, POSIX.1 */
	char* pw_gecos; /* Real Name or Comment field */
	char* pw_dir; /* Home directory, POSIX.1 */
	char* pw_shell; /* Shell Program, POSIX.1 */
};

struct spwd
{
	char* sp_namp; /* Login name */
	char* sp_pwdp; /* Encrypted password */
	long int sp_lstchg; /* Date of last change */
	long int sp_min; /* Minimum number of days between changes */
	long int sp_max; /* Maximum number of days between changes */
	long int sp_warn; /* Number of days to warn user to change the password */
	long int sp_inact; /* Number of days the account may be inactive */
	long int sp_expire; /* Number of days since 1970-01-01 until account expires */
	unsigned long int sp_flag; /* Reserved */
};

#endif

#ifndef uid_t
typedef __uid_t uid_t;
typedef __gid_t gid_t;
#endif

namespace gp {
	uint64_t toUInt(const njson& v, uint64_t de = 0);
	int64_t toInt(const njson& v, const char* k, int64_t de);
	int64_t toInt(const njson& v, int64_t de = 0);
	double toDouble(const njson& v, double de = 0);
	std::string toStr(const njson& v, const char* k, const std::string& des = "");
	std::string toStr(const njson& v, const std::string& des = "");
	int64_t str2int(const char* str, int64_t de = 0);
	njson str2ints(const std::string& s);
}

namespace hz {

	//#include "sftp_cmd.h"
	class fnh
	{
	public:
		std::string _ori;
		std::vector<std::string> _vs;
		int n = 0;
		std::string auth, group, user, name;
		int64_t size = 0;
	public:
		fnh()
		{
		}

		fnh(const std::string& s) :_ori(s)
		{
			_vs = md::split_m(s, " ", false);
			mk();
		}

		~fnh()
		{
		}
		void set(const std::string& s)
		{
			_ori = s;
			_vs = md::split_m(s, " ", false);
			mk();
		}
	public:
		void mk()
		{
			if (_vs.size() != 9)
			{
				return;
			}
			n = atoll(_vs[1].c_str());
			auth = _vs[0];
			group = _vs[3];
			user = _vs[2];
			name = _vs[8];
			if (n == 1)
			{
				size = atoll(_vs[4].c_str());
			}
			else if (n > 1)
			{
				name.push_back('/');
			}
		}
		void print()
		{
			auto t = _ori;
			auto last = t.find_last_of(' ');
			if (n > 1 && last != std::string::npos)
			{
				t.insert(last, "\x1b[34;1m");
				t += "\x1b[0m/";
			}
			printf("%s\n", t.c_str());
		}
	public:
		static int get_mon(const char* m);
	private:

	};

#ifndef HZ_BIT
	//位左移
#define HZ_BIT(x) (((uint64_t)1)<<(x))
#endif

	enum class permit_e :uint16_t
	{
		h_x = HZ_BIT(0), // 其他(H)
		h_w = HZ_BIT(1),
		h_r = HZ_BIT(2),
		g_x = HZ_BIT(3), // 组(G)
		g_w = HZ_BIT(4),
		g_r = HZ_BIT(5),
		o_x = HZ_BIT(6), // 拥有者(O)
		o_w = HZ_BIT(7),
		o_r = HZ_BIT(8),

		glue = HZ_BIT(9), // 粘附
		gid = HZ_BIT(10), // 设置GID
		uid = HZ_BIT(11), // 设置UID

		umask1 = HZ_BIT(12),
		umask2 = HZ_BIT(13),
		umask3 = HZ_BIT(14),
		umask4 = HZ_BIT(15),
	};

	static uint16_t operator| (const permit_e& p, const permit_e& p1)
	{
		return ((uint16_t)p | (uint16_t)p1);
	}
	static uint16_t operator| (const permit_e& p, uint16_t p1)
	{
		return ((uint16_t)p | (uint16_t)p1);
	}
	static uint16_t operator| (uint16_t p, const permit_e& p1)
	{
		return ((uint16_t)p | (uint16_t)p1);
	}
#define LOCK_K(lk) std::unique_lock<std::mutex> _lock_(lk);
	class ssh2_api //:public Singleton<ssh2_api>
	{
	private:
		int isinit = -1;
		std::mutex _lksi;
	public:
		ssh2_api()
		{
			isinit = libssh2_init(0);
		}

		~ssh2_api()
		{
			libssh2_exit();
		}
		LIBSSH2_SESSION* new_session()
		{
			LOCK_K(_lksi);
			LIBSSH2_SESSION* p = nullptr;
			if (isinit == 0)
			{
				p = libssh2_session_init();
			}
			return p;
		}
		void free_session(LIBSSH2_SESSION* p)
		{
			libssh2_session_disconnect(p, "Normal Shutdown");
			libssh2_session_free(p);
		}
	private:

	};

	class ssh_t
	{
	public:


		// 声明私钥解码回调函数
		using de_pkey_func = std::function<void(std::string* pkey, std::string& depkey)>;
		class rc_t
		{
		public:
			rc_t()
			{
			}

			virtual ~rc_t()
			{
			}

		private:

		};

		class channel_t :public rc_t
		{
		public:
			ssh_t* _t = 0;
			LIBSSH2_CHANNEL* _channel = 0;
			std::string _data;
			std::mutex _lkd;
		public:
			channel_t(ssh_t* p) :_t(p)
			{
			}

			~channel_t()
			{
				close();
			}
			LIBSSH2_CHANNEL* open(const std::string& termstr);
			int close();
			int exec(const std::string& cmdline);
			int write(const char* buf, size_t len);
			int write(const std::string& buf);
			int read(std::string* buf);
			const char* get_str(const std::string& last);
			std::string get_data();
			std::string* get_data_ptr();
		private:

		};
		class sftp_t :public rc_t
		{
		private:
			ssh_t* _ssh = nullptr;
			LIBSSH2_SFTP* _sftp_session = nullptr;
			std::mutex _lkd;
			// 未完成的文件后缀名
			std::string unfinished_suffix = ".__";
		public:
			sftp_t(ssh_t* s);

			~sftp_t();
			static sftp_t* create(ssh_t* s);
			void set_unfinished_suffix(const std::string& unfi);
			int ll(std::string sftppath, std::vector<std::string>* out);

			int ll(std::string sftppath, std::function<void(fnh*)> func);

			/*
			修改文件权限
			*/
			int remote_chmod(const std::string& remote, uint16_t permit, std::string* rcstr);
			/*
			修改远程主机指定文件的属主和属组
			*/
			int remote_chown(const std::string& remote, const std::string& user, const std::string& group, std::string* rcstr);
			/*
			上传文件
				1 lcoal         本地文件路径(包括名称)
				2 remote        要传输的文件在目标机器上的全路径
				3 pos*			继续上次的位置
				4 save_state	保存状态 return 非0则中断
								function参数：int64_t * offset当前已成功传送的偏移, int64_t total_send本次累计字节
								int64_t culen实时块大小, int64_t alen总大小
			*/
			int send_file(const std::string& local, const std::string& remote
				, int64_t* pos = nullptr, std::function<int(int64_t* offset, int64_t total_send, int64_t culen, int64_t alen)> save_state = nullptr);
			/*
			下载文件
			*/
			int recv_file(const std::string& local, const std::string& remote
				, int64_t* pos = nullptr, std::function<int(int64_t* offset, int64_t total_recv, int64_t culen, int64_t alen)> save_state = nullptr);
			/*
				重命名
				remote原文件名，remote_dst需设置的目标名
			*/
			int sftp_rename(const std::string& source_filename, const std::string& dest_filename);
		public:
			int sftp_init();
			int sftp_shutdown();
			int sftp_close_handle(LIBSSH2_SFTP_HANDLE* sftp_handle);
			LIBSSH2_SFTP_HANDLE* sftp_open(const std::string& path, long flags, long mode);
			int sftp_close(LIBSSH2_SFTP_HANDLE* sftp_handle);
			int sftp_read(LIBSSH2_SFTP_HANDLE* sftp_handle, char* buf, size_t len);
			int sftp_write(LIBSSH2_SFTP_HANDLE* sftp_handle, const char* buf, size_t len);
			LIBSSH2_SFTP_HANDLE* sftp_opendir(const std::string& path);
			int sftp_closedir(LIBSSH2_SFTP_HANDLE* sftp_handle);
			int sftp_readdir(LIBSSH2_SFTP_HANDLE* sftp_handle, char* buf, size_t buflen, LIBSSH2_SFTP_ATTRIBUTES* attrs);
			int sftp_unlink(const std::string& path);
			int sftp_mkdir(const std::string& path, long mode);
			int sftp_rmdir(const std::string& path);
			int sftp_stat(const std::string& path, LIBSSH2_SFTP_ATTRIBUTES* attrs);
			int sftp_lstat(const std::string& path, LIBSSH2_SFTP_ATTRIBUTES* attrs);
			int sftp_setstat(const std::string& path, LIBSSH2_SFTP_ATTRIBUTES* attrs);
			int sftp_readlink(const std::string& path, char* buf, size_t buflen);
			int sftp_symlink(const std::string& path, const std::string& link);
			int sftp_realpath(const std::string& path, char* buf, size_t buflen);
			void init_attrs(LIBSSH2_SFTP_ATTRIBUTES* attrs, struct stat* st);
			int sftp_copy_file(const std::string& local, const std::string& remote);
			int sftp_copy_link(const std::string& local, const std::string& remote);
			int sftp_copy_dir(const std::string& local, const std::string& remote);
			int sftp_copy(const std::string& local, const std::string& remote);
		private:
			/* forbidden copy */
			//ssh2_t(const ssh2_t& e);
			//ssh2_t& operator=(const ssh2_t& e);
			std::string basename(const std::string& path);
			std::string dirname(const std::string& path);
			std::string fixpath(const std::string& path);
		private:
		};

	protected:
		int _sock = 0, _auth_pw = 0;
		union {
			struct sockaddr_in sin;
			struct sockaddr_in6 sin6 = {};
		}ipv;
		// 指纹
		std::string _fingerprint;
		// 私钥文件、私钥数据
		std::string _privatekey_file, _privatekey;
		std::string _passphrase;
		de_pkey_func func_de;

		LIBSSH2_SESSION* _session = 0;
		int rc = 0;

		std::string _username, _password;
		std::string _host;
		std::string _cfn;
		njson jn;
		int _port = 22, _login_type = 4;
		// block=0非阻塞socket
		int _block = 1;
		int			m_errno = 0;    // m_errno>0的部分是等于系统errno, m_errno<0是libssh2的错误
		std::string m_errmsg;
		std::string term = "xterm-color";

		// 命令历史记录
		std::set<std::string> _cmds;

		std::mutex _lkrc;
		std::set<rc_t*> _all_c;
		ssh2_api* _ctx_api = nullptr;
		std::thread* _trc = nullptr, * _thrpoll = nullptr;
		std::atomic_bool _isrc = false;
		// 操作状态 1可以输入命令，2
		std::atomic_int _status = 0;
		std::atomic_int _stop = 0;
		// 命令数据锁
		std::mutex _lkcd;
		std::string _cmdstr, _last;
		std::vector<std::string> _cmdvs;
		channel_t* _ct = nullptr;
		std::function<void(ssh_t* pt, std::string* d)> _func_read;
		// 密码不对时重新输入
		std::function<std::string(ssh_t* pt)> _func_inputpw;
		std::function<std::string(ssh_t* pt)> _func_loadpk;
	public:
		ssh_t();

		~ssh_t();
	public:

		static ssh_t* create();
		auto getSession()
		{
			return _session;
		}
		int waitsocket(int timeout_ms = -1);
		void get_sys_error();
		void get_lib_error();
		void clear_error() { m_errno = 0; m_errmsg.clear(); }
		void disconnect();
		static ssh_t* run_test();
		static njson load_info(const std::string& fn);
	public:
		static ssh_t* new_run(njson oinfo, std::function<void(ssh_t* pt, std::string* d)> func_read);
		void set_func_auth(std::function<std::string(ssh_t* pt)> func_pw, std::function<std::string(ssh_t* pt)> func_pk);
		// 执行命令
		void send_cmd(const std::string& s);
		// 从控制台获取命令，创建运行线程
		void run_thr(bool iscmd);
		// 运行poll, -1阻塞
		void run_poll(int timeout);
		void run_cmd();
		njson get_info();
	public:
		void set_confname(const std::string& fn);
		int load_openfile();
		int load_conf();
		void save_conf();
		void set(const std::string& ip, const std::string& user, const std::string& pw, int port = 22, int block = 1);
		void set_pass(const std::string& pass);
		void set_auth(const std::string& privatekey_file);
		void set_auth_data(const std::string& privatekey, de_pkey_func func);
		int init();

		/*
		 * check what authentication methods are available
		 * 检查可用的身份验证方法
		 */
		void auth_list();
		// 验证用户
		int userauth(int auth_pw);
	private:

		static void kbd_callback(const char* name, int name_len,
			const char* instruction, int instruction_len, int num_prompts,
			const LIBSSH2_USERAUTH_KBDINT_PROMPT* prompts,
			LIBSSH2_USERAUTH_KBDINT_RESPONSE* responses,
			void** abstract);
	public:
		int exec(const std::string& cmdstr, std::string* out);

		LIBSSH2_CHANNEL* channel_open(const std::string& termstr);
		int channel_close(LIBSSH2_CHANNEL* channel);
		int channel_exec(LIBSSH2_CHANNEL* channel, const std::string& cmdline);
		int channel_poll(channel_t* ct, int timeout_ms, std::function<int(unsigned int t, bool is_out, bool is_read)> func);
		//int channel_poll(LIBSSH2_CHANNEL* channel, int timeout_ms, std::function<int(unsigned int t)> func);
		int channel_write(LIBSSH2_CHANNEL* channel, const char* buf, size_t len);
		int channel_read(LIBSSH2_CHANNEL* channel, std::string* buf);
		int channel_read_stderr(LIBSSH2_CHANNEL* channel, std::string* buf);
		channel_t* new_channel();
		void free_channel(channel_t* p);
		/*
		todo sftp文件操作
		*/
		sftp_t* new_sftp();
		void free_sftp(sftp_t* p);

		std::string uid_to_name(uid_t uid);
		std::string gid_to_name(gid_t gid);
		void set_nonblocking(int fd);

	};



#ifdef _WIN32
	HANDLE hout = 0;
	COORD coord = {};
	CONSOLE_SCREEN_BUFFER_INFO csbi; //控制台屏幕缓冲区信息
	void get_cpos()
	{
		GetConsoleScreenBufferInfo(hout, &csbi);
		coord.X = csbi.dwCursorPosition.X; //得到坐标X的值
		coord.Y = csbi.dwCursorPosition.Y; //得到坐标Y的值
	}
	void pos_inc(int x, int y)
	{
		coord.Y += y;
		coord.X += x;
		SetConsoleCursorPosition(hout, coord);
	}
#else
	void pos_inc(int x, int y)
	{
		char ud = y < 0 ? 'A' : 'B';
		if (x != 0)
			printf("\033[%d%c", (x), ud);
		char lr = y < 0 ? 'D' : 'C';
		if (y != 0)
			printf("\033[%d%c", (y), lr);
	}
#endif // _WIN32

	void save_fn(void* data, size_t len, std::string fn, int t)
	{
		//auto fn = File::getAP("tem.txt");
		//auto fd = fopen(fn.c_str(), "wb+");
		//if (fd)
		//{
		//	std::string_view buf = ".0123456789";
		//	auto ls = 29;// file_size(fd);
		//	int64_t ss = 0;
		//	int64_t n = 0;
		//	fseek64(fd, 5, SEEK_CUR);
		//	char rb[512] = {};
		//	while (n < ls)
		//	{
		//		//int nread = ::fread(rb + n, 1, 10, fd);
		//		//int nread = ::fwrite(buf.data(), 1, 10, fd);
		//		//
		//		if (nread > 0)
		//		{
		//			n += nread;
		//		}
		//	}
		//	fclose(fd);
		//	exit(1);
		//}

	}
	// todo save log
	void log_save(const std::string& d)
	{
		if (d.size())
		{
			hz::save_file("temp/ssh2tem.log", d.c_str(), d.size(), 0, true);
		}
	}
#ifdef _WIN32
	ssh_t* ssh_t::run_test()
	{
		hout = GetStdHandle(STD_OUTPUT_HANDLE);

		njson info;
		info["host"] = "127.0.0.1";
		info["login_name"] = "root";
		info["password"] = "";			// 密码登录
		info["port"] = 22;
		info["prikdata"];				// 私钥登录
		info["login_type"] = 1;
		auto ssh = new_run(info, [](ssh_t* pt, std::string* d) {
			if (d)
			{

			}
			});
		if (ssh)
		{
			ssh->run_thr(true);
		}
		return ssh;
		ssh_t* p = create();
		p->set_confname("tem.conf");
#if 0
		const std::string& ip = "39.108.66.40", user = "root", pw; int port = 22;
		p->set(ip, user, pw, port, 1);
#else 
		p->set("127.0.0.1", "root", "0.123456", 2202, 1);
#endif // 0
		struct stat st;
		auto ak = 0;// todo stat(File::getAP("tem.conf").c_str(), &st);
		p->init();
		std::string prikfile;
		bool isonce = false;
		int ret = -1;
		if (ret = p->load_conf() != 0)
		{
			ret = p->load_openfile();
		}
		//return p;
		if (ret == 0)
		{
			auto sftp = p->new_sftp();
			std::string rcstr[1];
			// 成功
			//sftp->remote_chmod("/home/a.xt", permit_e::o_r | permit_e::o_w | permit_e::g_r | permit_e::g_w, rcstr);
			//std::vector<std::future<int>> fes;
			//int cot = 100;
			//std::vector<std::future_status> pfs(cot, std::future_status::deferred);
			//for (int i = 0; i < cot; i++)
			//{
			//	std::future<int> future = std::async([=]() {
			//		//sftp->send_file(File::getAP("temp/ssh2tem.log"), "/home/ssh2tem" + std::to_string(i) + ".log");

			//		//new_ssh2session();
			//		//libssh2_init(0);
			//		//libssh2_session_init();
			//		//p->_ctx_api->new_session();
			//		//hz::sleep_ms(1000);
			//		return 0;
			//		});
			//	fes.push_back(std::move(future));
			//}
			////查询future的状态
			//for (int o = 0; o < cot;)
			//{
			//	int i = 0;
			//	for (auto& it : fes)
			//	{
			//		auto& status = pfs[i++];
			//		if (status != std::future_status::ready)
			//		{
			//			status = it.wait_for(std::chrono::milliseconds(500));
			//			if (status == std::future_status::deferred) {
			//				std::cout << "deferred\n";
			//			}
			//			else if (status == std::future_status::timeout) {
			//				std::cout << "timeout\n";
			//			}
			//			else if (status == std::future_status::ready) {
			//				std::cout << "ready!\n";
			//				printf("%d\n", o++);
			//			}
			//		}
			//	}
			//}
			//sftp->send_file(File::getAP("temp/ssh2tem.log"), "/home/ssh2tem.log");
			//sftp->recv_file(File::getAP("temp/temconf.txt"), "/home/a.xt");
			//printf("CHCP 65001\n");
			//printf("مرحباً \n");
			//printf("안녕하세요.");
		}
	}
#else

	ssh_t* ssh_t::run_test() {
		return 0;
	}
#endif
	njson ssh_t::load_info(const std::string& fn)
	{
		auto ninfo = hz::read_json(fn);
		njson info;
		try
		{
			if (ninfo.is_object())
			{
				auto idx = gp::toInt(ninfo["idx"], 0);
				njson& lst = ninfo["list"];
				if (lst.is_array() && lst.size())
				{
					if (idx < lst.size() && idx >= 0)
					{
						info = lst[idx];
					}
					else
					{
						info = *lst.rbegin();
					}
				}
				int tidx = gp::toInt(ninfo["term_idx"], 0);
				info["term"] = ninfo["terms"][tidx];
			}
		}
		catch (const std::exception& e)
		{
			printf("error:\t%s\n", e.what());
		}
		return info;
	}

	ssh_t* ssh_t::new_run(njson oinfo, std::function<void(ssh_t* pt, std::string* d)> func_read)
	{
		auto p = create();
		p->set(gp::toStr(oinfo["host"]), gp::toStr(oinfo["login_name"]), gp::toStr(oinfo["password"]), gp::toInt(oinfo["port"]), gp::toInt(oinfo["block"], 0));
		std::string pk = gp::toStr(oinfo["prikdata"]);
		std::string pkf = gp::toStr(oinfo["prikfile"]);
		std::string pass = gp::toStr(oinfo["pk_pass"]);
		p->_login_type = gp::toInt(oinfo["login_type"]);
		if (pk.size())
		{
			p->set_auth_data(pk, [](std::string* pkey, std::string& out) {
				// todo if (pkey)out = Base64::decode(*pkey);
				});
		}
		p->set_pass(pass);
		if (pkf.size())
		{
			p->set_auth(pkf);
		}
		if (0 == p->init())
		{
			p->_func_read = func_read;
			if (oinfo.find("term") != oinfo.end())
				p->term = gp::toStr(oinfo["term"]);
		}
		else {
			delete p; p = 0;
		}
		return p;
	}

	void ssh_t::set_func_auth(std::function<std::string(ssh_t* pt)> func_pw, std::function<std::string(ssh_t* pt)> func_pk)
	{
		LOCK_K(_lkcd);
		_func_inputpw = func_pw;
		_func_loadpk = func_pk;
	}

	void ssh_t::send_cmd(const std::string& s)
	{
		{
			LOCK_K(_lkcd);
			_cmdstr = s;
		}
		_status = 2;
	}

	void ssh_t::run_thr(bool iscmd)
	{
		int ret = -1;
		if (ret = userauth(_auth_pw) != 0)
		{
			return;
		}
		if (!_ct)
			_ct = new_channel();
		_ct->open(term);
		if (iscmd)
		{
			_trc = new std::thread([&]() {
				_isrc = true;
				while (_isrc)
				{
					run_cmd();
				}
				});
		}
		_thrpoll = new std::thread([=]() { run_poll(-1); _ct->close(); });
	}

	void ssh_t::run_poll(int timeout)
	{
		channel_poll(_ct, timeout, [&](unsigned int t, bool is_out, bool is_read) {
			if (is_read)
			{
				if (_func_read)
					_func_read(this, _ct->get_data_ptr());
				const char* tt = _ct->get_str(_last);
				printf("%s", tt);
				log_save(_ct->_data);
				channel_read_stderr(_ct->_channel, nullptr);
			}
			if (is_out && _status == 0)
			{
				_status = 1;
			}
			if (_status == 2 && _cmdstr.size())
			{
				_ct->write(_cmdstr);
				_status = 0; _cmdstr = "";
			}
			int r = _stop;
			return r;
			});
	}

	void ssh_t::run_cmd()
	{
		if (_status == 1)
		{
			std::string str;
			//std::cin >> str;
			char ch = 0;
			do
			{
				ch = getchar();
				str.push_back(ch);
			} while (ch != '\n');

			if (str.size())
			{
				_last = md::trim(str, "\n");
				send_cmd(str);
				str.clear();
			}
		}
	}

	njson ssh_t::get_info()
	{
		njson ret;
		ret["host"] = _host;
		ret["login_name"] = _username;
		ret["password"] = _password;
		ret["port"] = _port;
		ret["prikdata"] = _privatekey;
		ret["login_type"] = _login_type;
		return ret;
	}

	// 

	ssh_t::ssh_t()
	{
		static auto sctx = new ssh2_api();
		_ctx_api = sctx;
#ifdef WIN32
		static std::once_flag flag;
		static WSADATA wsadata;
		std::call_once(flag, [=]() {
			int err;
			err = WSAStartup(MAKEWORD(2, 2), &wsadata);
			if (err != 0) {
				fprintf(stderr, "WSAStartup failed with error: %d\n", err);
				return;
			}
			});
#endif

		//rc = libssh2_init(0);
		//if (rc != 0) {
		//	fprintf(stderr, "libssh2 initialization failed (%d)\n", rc);
		//	return;
		//}
	}

	ssh_t::~ssh_t()
	{
		disconnect();
	}

	void ssh_t::disconnect()
	{
		_isrc = false;
		if (_trc)
		{
			_trc->join();
			delete _trc;
			_trc = nullptr;
		}
		if (_thrpoll)
		{
			_thrpoll->join();
			delete _thrpoll;
			_thrpoll = nullptr;
		}
		{
			LOCK_K(_lkrc);
			for (auto pt : _all_c)
			{
				delete pt;
			}
			_all_c.clear();
		}
		if (_session)
		{
			_ctx_api->free_session(_session);
			_session = NULL;
		}
		if (_sock)
		{
#ifdef WIN32
			closesocket(_sock);
#else
			::close(_sock);
#endif
			_sock = 0;
		}
		libssh2_exit();
	}

	ssh_t* ssh_t::create()
	{
		return new ssh_t();
	}
	void ssh_t::set_confname(const std::string& fn)
	{
		_cfn = fn;
	}

	int ssh_t::load_openfile()
	{
		int ret = -1;
		std::string prikfile;
		if (_auth_pw & 4)
		{
			do
			{
				browse_openfile("选择私钥文件", "", "通用证书格式(*.pem)\0*.pem\0所有文件\0*.*\0putty(*.ppk)\0*.ppk\0", nullptr
					, [&prikfile](const std::vector<std::string>& fns) {
						if (fns.size())
							prikfile = fns[0];
					});
				set_auth(prikfile);
				ret = userauth(4);
				if (ret != 0)
				{
					int ok = MessageBoxA(0, "重新选择证书", "证书错误", MB_OKCANCEL);
					if (ok == 2)
						break;
				}
				else
				{
					save_conf();
				}
			} while (ret != 0);
		}
		return ret;
	}

	int ssh_t::load_conf()
	{
		int ret = -1;
		jn = read_json(_cfn);
		auto prikfile = gp::toStr(jn["prikfile"]);
		if (prikfile.size())
		{
			set_auth(prikfile);
			ret = userauth(_auth_pw);
		}
		return ret;
	}

	void ssh_t::save_conf()
	{
		if (_cfn != "")
		{
			jn["prikfile"] = _privatekey_file;
			save_json(jn, _cfn, -1);
		}
	}

	void ssh_t::set(const std::string& ip, const std::string& user, const std::string& pw, int port, int block)
	{
		_port = port;
		_block = block;
		_host = ip;
		_username = user;
		_password = pw;
	}

	void ssh_t::set_pass(const std::string& pass)
	{
		_passphrase = pass;
	}
	void ssh_t::set_auth(const std::string& privatekey_file)
	{
		_privatekey_file = privatekey_file;
	}
	void ssh_t::set_auth_data(const std::string& privatekey, de_pkey_func func)
	{
		_privatekey = privatekey;
		func_de = func;
	}

	void sleep_ms(int ms)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
	}

	int ssh_t::init()
	{
		unsigned long hostaddr = 0;
		if (_sock)
		{
			return 0;
		}
		/*
		* The application code is responsible for creating the socket
		* and establishing the connection
		*/
		if (_host.find(":") != std::string::npos)
		{
			_sock = socket(AF_INET6, SOCK_STREAM, 0);
			ipv.sin6.sin6_family = AF_INET6;
			ipv.sin6.sin6_port = htons(_port);
			inet_pton(AF_INET6, _host.c_str(), &ipv.sin6.sin6_addr);
			if (connect(_sock, (struct sockaddr*)(&ipv.sin6), sizeof(struct sockaddr_in6)) != 0) {
				// get_sys_error();
				fprintf(stderr, "failed to connect!\n");
				return -1;
			}
		}
		else
		{
			if (_host.size()) {
				hostaddr = inet_addr(_host.c_str());
			}
			else {
				hostaddr = htonl(0x7F000001);
			}
			_sock = socket(AF_INET, SOCK_STREAM, 0);
			ipv.sin.sin_family = AF_INET;
			ipv.sin.sin_port = htons(_port);
			ipv.sin.sin_addr.s_addr = hostaddr;
			if (connect(_sock, (struct sockaddr*)(&ipv.sin),
				sizeof(struct sockaddr_in)) != 0) {
				// get_sys_error();
				fprintf(stderr, "failed to connect!\n");
				return -1;
			}
		}


		// Create a session instance
		_session = _ctx_api->new_session();
		if (!_session)
			return -1;
		/* Since we have set non-blocking, tell libssh2 we are non-blocking */
		libssh2_session_set_blocking(_session, _block);

		if (_block)
		{
			rc = libssh2_session_handshake(_session, _sock);
		}
		else
		{
			waitsocket();
			// 非阻塞socket
			/* ... start it up. This will trade welcome banners, exchange keys,
			* and setup crypto, compression, and MAC layers
			*/
			while ((rc = libssh2_session_handshake(_session, _sock)) == LIBSSH2_ERROR_EAGAIN)
			{
				int t = waitsocket();
				if (t < 0)
					break;
				sleep_ms(1);
			}
		}
		if (rc) {
			fprintf(stderr, "Failure establishing SSH session: %d\n", rc);
			return -1;
		}

		/* At this point we havn't yet authenticated.  The first thing to do is
		* check the hostkey's fingerprint against our known hosts Your app may
		* have it hard coded, may go to a file, may present it to the user,
		* that's your call
		*/
		const char* fingerprint = nullptr;
		do
		{
			fingerprint = libssh2_hostkey_hash(_session, LIBSSH2_HOSTKEY_HASH_SHA1);

		} while (!fingerprint);
		_fingerprint.resize(20);
		memcpy((char*)_fingerprint.data(), fingerprint, 20);
		//fprintf(stderr, "Fingerprint: ");
		//for (int i = 0; i < 20; i++) {
		//	fprintf(stderr, "%02X ", (unsigned char)fingerprint[i]);
		//}
		//fprintf(stderr, "\n");
		auth_list();
		return 0;
	}

	// 验证用户


	/*
	* check what authentication methods are available
	* 检查可用的身份验证方法
	*/

	void ssh_t::auth_list()
	{
		char* userauthlist = NULL;
		while ((userauthlist = libssh2_userauth_list(_session, _username.c_str(), _username.size())) == NULL
			&& libssh2_session_last_error(_session, NULL, NULL, 0) == LIBSSH2_ERROR_EAGAIN
			&& waitsocket() >= 0);
		if (!userauthlist)return;
		if (strstr(userauthlist, "password") != NULL)
		{
			_auth_pw |= 1;
		}
		if (strstr(userauthlist, "keyboard-interactive") != NULL)
		{
			_auth_pw |= 2;
		}
		if (strstr(userauthlist, "publickey") != NULL)
		{
			_auth_pw |= 4;
		}
	}

	int ssh_t::userauth(int auth_pw)
	{
		do
		{
			if (auth_pw & 1 && (_login_type == 0 || _login_type == 1))
			{
				/* We could authenticate via password */
				std::string pw = _password;
				do
				{
					while ((rc = libssh2_userauth_password(_session, _username.c_str(), pw.c_str()))
						== LIBSSH2_ERROR_EAGAIN && waitsocket() >= 0);
					if (rc != 0)
					{
						get_lib_error();
						fprintf(stderr, "Authentication by password failed.\n");
						if (_func_inputpw)
						{
							pw = _func_inputpw(this);
						}
					}
				} while (rc != 0);
				break;
			}
			if (_login_type == 0 || _login_type == 3)
			{
				if (auth_pw & 2)
				{
					/* Or via keyboard-interactive */
					while ((rc = libssh2_userauth_keyboard_interactive(_session, _username.c_str(), &kbd_callback))
						== LIBSSH2_ERROR_EAGAIN && waitsocket() >= 0);
					if (rc != 0)
					{
						get_lib_error();
						fprintf(stderr, "Authentication by keyboard-interactive failed!\n");
						return -2;
					}
					else {
						//fprintf(stderr, "Authentication by keyboard-interactive succeeded.\n");
					}
					break;
				}
			}
			if (_login_type == 0 || _login_type == 2)
			{
				if (auth_pw & 4)
				{
					/* Or by public key */
					std::string prikfile = _privatekey_file;
					char* passphrase = 0;
					if (_passphrase.size())
					{
						passphrase = (char*)_passphrase.c_str();
					}
#ifdef _WIN32
					/*	if (prikfile.size() && prikfile[1] != ':')
							prikfile = File::getAP(prikfile);*/
#endif // _WIN32
					do {
						while ((rc = libssh2_userauth_publickey_fromfile(_session, _username.c_str(), 0, prikfile.c_str(), passphrase))
							== LIBSSH2_ERROR_EAGAIN && waitsocket() >= 0);
						if (rc != 0)
						{
							get_lib_error();
							fprintf(stderr, "Authentication by public key failed!\n");
							if (_func_loadpk)
							{
								prikfile = _func_loadpk(this);
								if (prikfile.empty())
								{
									break;
								}
							}
						}
						else {
							//fprintf(stderr, "Authentication by public key succeeded.\n");
							// todo auto pk = File::read_binary_file(prikfile);
							//pk.push_back(0);
							//_privatekey = Base64::encode(pk.data());
						}
					} while (rc != 0);
					break;
				}
				if (auth_pw & 4 && _privatekey.size())
				{
					std::string pkey = _privatekey;
					if (func_de)
					{
						func_de(&_privatekey, pkey);
					}
					char* passphrase = 0;
					if (_passphrase.size())
					{
						passphrase = (char*)_passphrase.c_str();
					}
					while ((rc = libssh2_userauth_publickey_frommemory(_session, _username.c_str(), strlen(_username.c_str()), 0, 0, pkey.data(), pkey.size(), passphrase))
						== LIBSSH2_ERROR_EAGAIN && waitsocket() >= 0);
					if (rc != 0)
					{
						get_lib_error();
						fprintf(stderr, "Authentication by public key failed!\n");
					}
					else {
						//fprintf(stderr, "Authentication by public key succeeded.\n");
					}
					break;
				}
			}

		} while (0);
		save_conf();
		return rc;
	}

	void ssh_t::kbd_callback(const char* name, int name_len, const char* instruction, int instruction_len, int num_prompts, const LIBSSH2_USERAUTH_KBDINT_PROMPT* prompts, LIBSSH2_USERAUTH_KBDINT_RESPONSE* responses, void** _abstract)
	{
		int i;
		size_t n;
		char buf[1024];
		(void)_abstract;

		fprintf(stderr, "Performing keyboard-interactive authentication.\n");

		fprintf(stderr, "Authentication name: '");
		fwrite(name, 1, name_len, stderr);
		fprintf(stderr, "'\n");

		fprintf(stderr, "Authentication instruction: '");
		fwrite(instruction, 1, instruction_len, stderr);
		fprintf(stderr, "'\n");

		fprintf(stderr, "Number of prompts: %d\n\n", num_prompts);

		for (i = 0; i < num_prompts; i++) {
			fprintf(stderr, "Prompt %d from server: '", i);
			fwrite(prompts[i].text, 1, prompts[i].length, stderr);
			fprintf(stderr, "'\n");

			fprintf(stderr, "Please type response: ");
			fgets(buf, sizeof(buf), stdin);
			n = strlen(buf);
			while (n > 0 && strchr("\r\n", buf[n - 1]))
				n--;
			buf[n] = 0;

			responses[i].text = strdup(buf);
			responses[i].length = n;

			fprintf(stderr, "Response %d from user is '", i);
			fwrite(responses[i].text, 1, responses[i].length, stderr);
			fprintf(stderr, "'\n\n");
		}

		fprintf(stderr,
			"Done. Sending keyboard-interactive responses to server now.\n");
	}


	int ssh_t::exec(const std::string& cmdline, std::string* out)
	{
		int rc = 0;
		int ret = 0;
		LIBSSH2_CHANNEL* channel = NULL;
		while ((channel = libssh2_channel_open_session(_session)) == NULL
			&& libssh2_session_last_error(_session, NULL, NULL, 0) == LIBSSH2_ERROR_EAGAIN
			&& waitsocket() >= 0);
		if (channel == NULL)
		{
			return -1;
		}
		libssh2_channel_set_blocking(channel, 1);
		while ((rc = libssh2_channel_exec(channel, cmdline.c_str())) == LIBSSH2_ERROR_EAGAIN && waitsocket() >= 0);
		if (rc)
		{
			ret = -1;
		}
		_cmds.insert(cmdline);
		if (0 == rc)
		{
			char buf[256] = { 0 };
			std::string cr;
			if (!out)
			{
				out = &cr;
			}
			while (1)
			{
				int nread = libssh2_channel_read(channel, buf, 256);
				if (LIBSSH2_ERROR_EAGAIN == nread)
				{
					continue;
				}
				if (nread > 0)
				{
					out->append(buf, nread);
				}
				else
				{
					break;
				}
			} // end while (1)
			printf("%s", out->c_str());
		}

		while (channel && (rc = libssh2_channel_close(channel)) == LIBSSH2_ERROR_EAGAIN && waitsocket() >= 0);
		if (channel && rc == 0)
		{
			ret = libssh2_channel_get_exit_status(channel);
			libssh2_channel_free(channel);
			channel = NULL;
		}
		return 0;
	}
	LIBSSH2_CHANNEL* ssh_t::channel_open(const std::string& termstr)
	{
		LIBSSH2_CHANNEL* channel = NULL;
		while ((channel = libssh2_channel_open_session(_session)) == NULL
			&& libssh2_session_last_error(_session, NULL, NULL, 0) == LIBSSH2_ERROR_EAGAIN
			&& waitsocket() >= 0);
		if (channel == NULL)
		{
			get_sys_error();
		}
		else
		{
			libssh2_channel_set_blocking(channel, 0);
		}
		if (termstr.size())
		{
			libssh2_channel_request_pty(channel, termstr.data());
		}
		auto hr = libssh2_channel_shell(channel);
		return channel;
	}
	int ssh_t::channel_close(LIBSSH2_CHANNEL* channel)
	{
		int rc = 0, ret = 0;
		while (channel && (rc = libssh2_channel_close(channel)) == LIBSSH2_ERROR_EAGAIN && waitsocket() >= 0);
		if (channel && rc == 0)
		{
			ret = libssh2_channel_get_exit_status(channel);
			libssh2_channel_free(channel);
		}
		return  ret;
	}
	int ssh_t::channel_exec(LIBSSH2_CHANNEL* channel, const std::string& cmdline)
	{
		int rc = 0;
		while ((rc = libssh2_channel_exec(channel, cmdline.c_str())) == LIBSSH2_ERROR_EAGAIN && waitsocket() >= 0);
		if (rc)
		{
			get_lib_error();
			return -1;
		}
		_cmds.insert(cmdline);
		return libssh2_channel_get_exit_status(channel);
	}

	int ssh_t::channel_poll(channel_t* ct, int timeout_ms, std::function<int(unsigned int t, bool is_out, bool is_read)> func)
	{
		if (!ct || !ct->_channel)
		{
			return -1;
		}
		LIBSSH2_CHANNEL* channel = ct->_channel;
		LIBSSH2_POLLFD fds[2] = {};
		for (size_t i = 0; i < 2; i++)
		{
			fds[i].type = LIBSSH2_POLLFD_CHANNEL;
			fds[i].fd.channel = channel;
			fds[i].events = i == 0 ? LIBSSH2_POLLFD_POLLIN : LIBSSH2_POLLFD_POLLOUT;
		}
		int bk = 0;
		int inc = -1;
		if (timeout_ms == -1) {
			inc = 0; timeout_ms = 1;
		}
		while (timeout_ms > 0)
		{
			int rc = libssh2_poll(fds, 2, 10);
			if (rc < 1)
			{
				if (timeout_ms > 0)
				{
					timeout_ms += inc;
					hz::sleep_ms(1);
					continue;
				}
				else {
					break;
				}
			}
			bool is_read = false;
			if (fds[0].revents & LIBSSH2_POLLFD_POLLIN)
			{
				LOCK_K(ct->_lkd);
				ct->_data.clear();
				// 获取数据
				int rc = 0;
				do
				{
					rc = ct->read(&ct->_data);
				} while (rc > 0);
				is_read = !ct->_data.empty();
			}
			if (func)
			{
				bk = func(fds[0].revents | fds[1].revents, fds[1].revents & LIBSSH2_POLLFD_POLLOUT, is_read);
			}
			if (bk)
			{
				break;
			}
			hz::sleep_ms(1);
		}
		return 0;
	}

	int ssh_t::channel_write(LIBSSH2_CHANNEL* channel, const char* buf, size_t len)
	{
		int ret = 0, rn = 0;
		while (len)
		{
			while ((rn = libssh2_channel_write(channel, buf, len)) == LIBSSH2_ERROR_EAGAIN && waitsocket() >= 0);
			if (rn > 0)
			{
				ret += rn;
				buf += rn;
				len -= rn;
			}
			else
			{
				break;
			}
		}
		get_lib_error();
		return ret;
	}
	int ssh_t::channel_read(LIBSSH2_CHANNEL* channel, std::string* obuf)
	{
		int ret = 0, nread = 0;
		std::string tem;
		if (!obuf)
		{
			obuf = &tem;
		}
		char buf[1024] = {};
#if 1
		nread = libssh2_channel_read(channel, buf, 1024);
		if (nread > 0)
		{
			//printf(buf);
			obuf->append(buf, nread);
		}
#else
		while (1)
		{
			while ((nread = libssh2_channel_read(channel, buf, 512)) == LIBSSH2_ERROR_EAGAIN && waitsocket() >= 0);
			if (LIBSSH2_ERROR_EAGAIN == nread)
			{
				continue;
			}
			if (nread > 0)
			{
				printf(buf);
				obuf->append(buf, nread);
			}
			else
			{
				break;
			}
		}

#endif // 1

		get_lib_error();
		return nread;
	}
	int ssh_t::channel_read_stderr(LIBSSH2_CHANNEL* channel, std::string* obuf)
	{
		char bufe[1024] = {};
		auto nread = libssh2_channel_read_stderr(channel, bufe, 1024);
		if (nread > 0)
		{
			printf("stderr:\t");
			printf(bufe);
		}
		if (obuf && nread > 0)
		{
			obuf->append(bufe, nread);
		}
		return nread;
	}
	ssh_t::channel_t* ssh_t::new_channel()
	{
		channel_t* p = new channel_t(this);
		LOCK_K(_lkrc);
		_all_c.insert(p);
		return p;
	}
	void ssh_t::free_channel(channel_t* p)
	{
		if (p)
		{
			LOCK_K(_lkrc);
			_all_c.erase(p);
			delete p;
		}
	}

	ssh_t::sftp_t* ssh_t::new_sftp()
	{
		auto p = ssh_t::sftp_t::create(this);
		LOCK_K(_lkrc);
		_all_c.insert(p);
		return p;
	}

	void ssh_t::free_sftp(sftp_t* p)
	{
		if (p)
		{
			LOCK_K(_lkrc);
			_all_c.erase(p);
			delete p;
		}
	}


	int ssh_t::waitsocket(int timeout_ms/*=-1*/)
	{
		int rc;
#ifdef _WIN32
		struct timeval timeout;
		struct fd_set fd;
		fd_set* writefd = NULL;
		fd_set* readfd = NULL;
		int dir;

		timeout.tv_sec = timeout_ms;
		timeout.tv_usec = 0;

		FD_ZERO(&fd);

		FD_SET(_sock, &fd);

		/* now make sure we wait in the correct direction */
		dir = libssh2_session_block_directions(_session);

		if (dir & LIBSSH2_SESSION_BLOCK_INBOUND)
			readfd = &fd;

		if (dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
			writefd = &fd;


		rc = select(_sock + 1, readfd, writefd, NULL, &timeout);
#else

		struct pollfd pfd;
		memset(&pfd, 0, sizeof(pfd));
		pfd.events = 0; //POLLHUP | POLLERR;
		pfd.fd = _sock;
		int dir = 0;
		// now make sure we wait in the correct direction
		dir = libssh2_session_block_directions(_session);
		if (dir & LIBSSH2_SESSION_BLOCK_INBOUND)
			pfd.events |= POLLIN;
		if (dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
			pfd.events |= POLLOUT;
		rc = poll(&pfd, 1, timeout_ms * 1000);
#endif // _WIN32
		return rc;
	}
	void ssh_t::get_sys_error()
	{
		LOCK_K(_lkrc);
		m_errno = errno;
		//m_errmsg=strerror(m_errno);
		char buf[512] = {};
		int s = strerror_r(m_errno, buf, 512);
		if (buf[0])
			m_errmsg = buf;
	}
	void ssh_t::get_lib_error()
	{
		LOCK_K(_lkrc);
		m_errno = libssh2_session_last_errno(_session);
		char* errmsg = NULL;
		int errmsg_len = 0;
		libssh2_session_last_error(_session, &errmsg, &errmsg_len, 0);
		if (errmsg_len > 0 && errmsg)
		{
			m_errmsg.assign(errmsg, errmsg_len);
		}
	}

	std::string ssh_t::uid_to_name(uid_t uid)
	{
		std::string name;
		struct passwd pwd, * pret = NULL;
#ifndef _WIN32
		char buf[512] = {};
		if (getpwuid_r(uid, &pwd, buf, 512, &pret) == 0 && pret == &pwd && pwd.pw_name)
		{
			name = pwd.pw_name;
		}
#endif // !_WIN32
		return name;
	}
	std::string ssh_t::gid_to_name(gid_t gid)
	{
		std::string name;
#ifndef _WIN32
		char buf[512] = {};
		struct group grp, * pret = NULL;
		if (getgrgid_r(gid, &grp, buf, 512, &pret) == 0 && pret == &grp && grp.gr_name)
		{
			name = grp.gr_name;
		}
#endif // !_WIN32
		return name;
	}
	/* set a fd into nonblocking mod#include <grp.h>e. */
	void ssh_t::set_nonblocking(int fd)
	{
		int val;
#ifdef FCNTL
		if ((val = fcntl(fd, F_GETFL)) == -1)
			return;
		if (!(val & O_NONBLOCK)) {
			val |= O_NONBLOCK;
			fcntl(fd, F_SETFL, val);
		}
#endif
	}
	// -----------------------------------------------------------------------------------------------------------------------
	// todo class fnh 
	int fnh::get_mon(const char* m)
	{
		static std::unordered_map<std::string, int> ms = {
			{"Jan",0},{"Feb",1},{"Mar",2},{"Apr",3},{"May",4},{"Jun",5},{"Jul",6}
			,{"Aug",7},{"Sep",8},{"Otc",9},{"Nov",10},{"Dec",11}
		};
		auto it = ms.find(m);
		return it != ms.end() ? it->second + 1 : -1;
	}
	LIBSSH2_CHANNEL* ssh_t::channel_t::open(const std::string& str)
	{
		return _channel = _t->channel_open(str);
	}
	int ssh_t::channel_t::close()
	{
		int ret = _t->channel_close(_channel); _channel = 0;
		return ret;
		//_t->free_channel(this);
	}
	int ssh_t::channel_t::exec(const std::string& cmdline)
	{
		return _t->channel_exec(_channel, cmdline);
	}
	int ssh_t::channel_t::write(const char* buf, size_t len)
	{
		return _t->channel_write(_channel, buf, len);
	}
	int ssh_t::channel_t::write(const std::string& buf)
	{
		return _t->channel_write(_channel, buf.c_str(), buf.size());
	}
	int ssh_t::channel_t::read(std::string* buf)
	{
		return _t->channel_read(_channel, buf);
	}
	const char* ssh_t::channel_t::get_str(const std::string& last)
	{
		//LOCK_R(_lkd);
		//printf("\x1b[32;1m%s\x1b[0m\n", "r");
		const char* tt = _data.c_str();
		if (!last.empty())
		{
#if 0
			pos_inc(last.size(), -1);
			last = "";
#else
			int cpe = _data.find(last);
			if (cpe == 0)
			{
				tt += last.size();
				for (; *tt != '\n'; tt++);
				tt++;
			}
#endif
		}
		return tt;
	}
	std::string* ssh_t::channel_t::get_data_ptr()
	{
		//LOCK_R(_lkd);
		return &_data;
	}
	std::string ssh_t::channel_t::get_data()
	{
		//LOCK_R(_lkd);
		return _data;
	}
	// todo sftp

	ssh_t::sftp_t::sftp_t(ssh_t* s) :_ssh(s)
	{
		sftp_init();
	}

	ssh_t::sftp_t::~sftp_t()
	{
		sftp_shutdown();
	}

	ssh_t::sftp_t* ssh_t::sftp_t::create(ssh_t* s)
	{
		return s ? new sftp_t(s) : nullptr;
	}

	void ssh_t::sftp_t::set_unfinished_suffix(const std::string& unfi)
	{
		unfinished_suffix = unfi;
	}

	int ssh_t::sftp_t::sftp_rename(const std::string& source_filename, const std::string& dest_filename)
	{
		long flag = LIBSSH2_SFTP_RENAME_OVERWRITE | LIBSSH2_SFTP_RENAME_ATOMIC | LIBSSH2_SFTP_RENAME_NATIVE;
		return libssh2_sftp_rename_ex(_sftp_session, source_filename.c_str(), source_filename.size(), dest_filename.c_str(), dest_filename.size(), flag);
	}

	int ssh_t::sftp_t::sftp_init()
	{
		while ((_sftp_session = libssh2_sftp_init(_ssh->_session)) == NULL
			&& libssh2_session_last_error(_ssh->_session, NULL, NULL, 0) == LIBSSH2_ERROR_EAGAIN
			&& _ssh->waitsocket() >= 0);
		if (!_sftp_session) {
			_ssh->get_lib_error();
			fprintf(stderr, "Unable to init SFTP session\n");
			return -1;
		}
		return 0;
	}
#if 0
	int ssh_t::sftp_t::open(const std::string& sftppath)
	{
		fprintf(stderr, "libssh2_sftp_open()!\n");
		/* Request a file via SFTP */
		auto sftp_handle = libssh2_sftp_open(_sftp_session, sftppath.c_str(), LIBSSH2_FXF_READ, 0);

		if (!sftp_handle) {
			fprintf(stderr, "Unable to open file with SFTP: %ld\n",
				libssh2_sftp_last_error(_sftp_session));
			return -1;
		}
		int rc = 0;
		do {
			char mem[1024];

			/* loop until we fail */
			fprintf(stderr, "libssh2_sftp_read()!\n");
			rc = libssh2_sftp_read(sftp_handle, mem, sizeof(mem));
			if (rc > 0) {
				write(1, mem, rc);
			}
			else {
				break;
			}
		} while (1);
		return rc;
	}
#endif
	int ssh_t::sftp_t::ll(std::string sftppath, std::vector<std::string>* out)
	{
		return ll(sftppath, [=](fnh* p) {
			if (out)
			{
				out->push_back(p->name);
			}
			});
	}
	int ssh_t::sftp_t::ll(std::string sftppath, std::function<void(fnh*)> func)
	{
		int rc = 0;
		if (sftppath.empty())
		{
			return rc;
		}
		LIBSSH2_SFTP_HANDLE* sftp_handle = nullptr;
		/* Request a dir listing via SFTP */
		if (sftppath[sftppath.size()] != '/')
		{
			sftppath.push_back('/');
		}
		while ((sftp_handle = libssh2_sftp_opendir(_sftp_session, sftppath.c_str())) == NULL
			&& libssh2_session_last_errno(_ssh->_session) == LIBSSH2_ERROR_EAGAIN
			&& _ssh->waitsocket() >= 0);
		if (!sftp_handle) {
			fprintf(stderr, "Unable to open dir with SFTP\n");
			return -1;
		}
		//fprintf(stderr, "libssh2_sftp_opendir() is done, now receive listing!\n");
		if (!func)
		{
			func = [](fnh* p) { p->print(); };
		}
		fnh f;
		do {
			char mem[512];
			char longentry[512];
			LIBSSH2_SFTP_ATTRIBUTES attrs;

			/* loop until we fail */
			//while ((rc = libssh2_sftp_readdir(_sftp_handle, mem, sizeof(mem), &attrs)) == LIBSSH2_ERROR_EAGAIN && waitsocket() >= 0);
			while ((rc = libssh2_sftp_readdir_ex(sftp_handle, mem, sizeof(mem),
				longentry, sizeof(longentry), &attrs)) == LIBSSH2_ERROR_EAGAIN
				&& _ssh->waitsocket() >= 0);
			if (rc > 0) {
				/* rc is the length of the file name in the mem
				buffer */

				if (longentry[0] != '\0') {
					int fns = gp::str2int(longentry);
					f.set(longentry);
					func(&f);
				}
				else {
					if (attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) {
						/* this should check what permissions it
						is and print the output accordingly */
						printf("--fix----- ");
					}
					else {
						printf("---------- ");
					}

					if (attrs.flags & LIBSSH2_SFTP_ATTR_UIDGID) {
						printf("%4ld %4ld ", attrs.uid, attrs.gid);
					}
					else {
						printf("   -    - ");
					}

					if (attrs.flags & LIBSSH2_SFTP_ATTR_SIZE) {
						printf("%8" PRIu64 " ", attrs.filesize);
					}

					printf("%s\n", mem);
				}
			}
			else
				break;

		} while (1);

		libssh2_sftp_close(sftp_handle);
		return rc;
	}
	/*
	sftp 文件
	设置UID、设置GID、粘附 -> 111
	拥有者(O)、组(G)、其他(H)
	rwx -> 111
	*/
	int ssh_t::sftp_t::remote_chmod(const std::string& remote, uint16_t permit, std::string* rcstr)
	{
		char buf[128] = {};
		auto s = std::string("chmod ") + itoa(permit, buf, 8) + " " + remote;
		return _ssh->exec(s, rcstr);
	}
	int ssh_t::sftp_t::remote_chown(const std::string& remote, const std::string& user, const std::string& group, std::string* rcstr)
	{
		std::string cmdline = std::string("chown ") + user + ":" + group + " " + remote;
		int ret = _ssh->exec(cmdline, rcstr);
		return ret;
	}

	int ssh_t::sftp_t::send_file(const std::string& local, const std::string& remote
		, int64_t* pos, std::function<int(int64_t* offset, int64_t total, int64_t culen, int64_t alen)> save_state)
	{
		off_t total_send = 0, offsets;
		std::string errbuf[1];
		struct stat st;
		_ssh->clear_error();
		if (stat(local.c_str(), &st))
		{
			_ssh->get_sys_error();
			return -2;
		}
		int ret = 0;
		FILE* fd = 0;  // local file fd
		LIBSSH2_SFTP_HANDLE* sftp_handle = NULL;
		int64_t lpos = 0;
		if (!pos)
		{
			pos = &lpos;
		}
		offsets = *pos;
		do
		{
			if (NULL == _sftp_session)
			{
				_ssh->get_lib_error();
				ret = -3;
				break;
			}
			while ((sftp_handle = libssh2_sftp_open(_sftp_session, (remote + unfinished_suffix).c_str(),
				LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
				st.st_mode)) == NULL
				&& libssh2_session_last_errno(_ssh->_session) == LIBSSH2_ERROR_EAGAIN);
			if (NULL == sftp_handle)
			{
				ret = -1;
				_ssh->get_lib_error();
				break;

			}
			fd = fopen(local.c_str(), "rb");
			if (!fd)
			{
				ret = -1;
				_ssh->get_sys_error();
				break;
			}
			else
			{
				std::vector<char> buf;
				auto ls = md::file_size(fd);
				int64_t ss = 0;
				buf.resize(1024);
				fseeki64(fd, offsets, SEEK_SET);
				libssh2_sftp_seek64(sftp_handle, offsets);
				while (1)
				{
					int nread = ::fread(buf.data(), 1, 1024, fd);
					if (nread <= 0)
					{
						if (nread < 0)
						{
							// read出错
							_ssh->get_sys_error();
						}
						break;
					}
					ss += nread;
					int cun = nread;
					char* write_ptr = buf.data();
					//
					while (nread > 0)
					{
						int nwrite = libssh2_sftp_write(sftp_handle, write_ptr, nread);
						if (LIBSSH2_ERROR_EAGAIN == nwrite)
						{
							continue;
						}
						if (nwrite < 0)
						{
							break;
						}
						else
						{
							total_send += nwrite;
							nread -= nwrite;
							write_ptr += nwrite;
							*pos += nwrite;
							// 发送成功cun
							if (save_state)
							{
								ret = save_state(pos, total_send, nwrite, ls);
								if (ret != 0)
								{
									break;
								}
							}
						}
					}
					// 仍有未写入的序列,中断传输, 则出错推出循环
					if (nread || ret != 0)
					{
						_ssh->get_lib_error();
						break;
					}
				}
				if (total_send + offsets < st.st_size)
				{
					ret = -1;
				}
				::fclose(fd);
			}
		} while (0);
		while (sftp_handle && libssh2_sftp_close(sftp_handle) == LIBSSH2_ERROR_EAGAIN);
		if (ret == 0)
		{
			if (total_send + offsets == st.st_size && unfinished_suffix.size())
			{
				sftp_rename(remote + unfinished_suffix, remote);
			}
			//if (keep_owner)
			{
				//remote_chown(remote, uid_to_name(st.st_uid), gid_to_name(st.st_gid), nullptr);
			}
			//remote_chmod(remote, st.st_mode, errbuf);
		}
		sftp_handle = NULL;
		return ret;
	}
	int ssh_t::sftp_t::recv_file(const std::string& local, const std::string& remote
		, int64_t* pos, std::function<int(int64_t* offset, int64_t total, int64_t culen, int64_t alen)> save_state)
	{
		off_t total_recv = 0, offsets;
		LIBSSH2_SFTP_ATTRIBUTES attrs;
		struct stat st;
		LIBSSH2_SFTP_HANDLE* sftp_handle = NULL;
		int rc = 0;
		FILE* fd = 0;
		int ret = 0;
		int64_t lpos = 0;
		if (!pos)
		{
			pos = &lpos;
		}
		offsets = *pos;
		_ssh->clear_error();

		if (NULL == _sftp_session)
		{
			_ssh->get_lib_error();
			return -1;
		}
		do
		{
			if (NULL == _sftp_session)
			{
				_ssh->get_lib_error();
				break;
			}
			while ((sftp_handle = libssh2_sftp_open(_sftp_session, remote.c_str(), LIBSSH2_FXF_READ, 0)) == NULL
				&& libssh2_session_last_errno(_ssh->_session) == LIBSSH2_ERROR_EAGAIN);
			if (NULL == sftp_handle)
			{
				ret = -1;
				_ssh->get_lib_error();
				break;
			}
			while ((rc = libssh2_sftp_stat(_sftp_session, remote.c_str(), &attrs) == LIBSSH2_ERROR_EAGAIN));
			if (rc)
			{
				ret = -1;
				_ssh->get_lib_error();
				break;
			}
			else
			{
				// FIX ME : 未检查是否为文件
				st.st_size = attrs.flags & LIBSSH2_SFTP_ATTR_SIZE ? attrs.filesize : 0;
				st.st_mode = attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS ? attrs.permissions : 0644;
				st.st_atime = attrs.flags & LIBSSH2_SFTP_ATTR_ACMODTIME ? attrs.atime : time(NULL);
				st.st_mtime = attrs.flags & LIBSSH2_SFTP_ATTR_ACMODTIME ? attrs.mtime : time(NULL);
				fd = fopen((local + unfinished_suffix).c_str(), "wb+");// O_RDWR | O_CREAT | O_TRUNC, st.st_mode);
			}
			if (fd)
			{
				std::vector<char> buf;
				buf.resize(1024);
				fseeki64(fd, offsets, SEEK_SET);
				libssh2_sftp_seek64(sftp_handle, offsets);
				while (1)
				{
					int nread = libssh2_sftp_read(sftp_handle, buf.data(), 1024);
					if (LIBSSH2_ERROR_EAGAIN == nread)
					{
						_ssh->waitsocket();
						continue;
					}
					if (nread <= 0)
					{
						if (nread < 0)
						{
							// libssh2_channel_read错误
							ret = -1;
							_ssh->get_lib_error();
						}
						break;
					}
					char* write_ptr = buf.data();
					while (nread > 0)
					{
						int nwrite = ::fwrite(write_ptr, 1, nread, fd);
						if (nwrite < 0)
						{
							break;
						}
						else
						{
							total_recv += nwrite;
							nread -= nwrite;
							write_ptr += nwrite;

							*pos += nwrite;
							// 发送成功cun
							if (save_state)
							{
								ret = save_state(pos, total_recv, nwrite, st.st_size);
								if (ret != 0)
								{
									break;
								}
							}
						}
					}
					if (nread || ret != 0)
					{
						_ssh->get_sys_error();
						break;
					}
				}
				if (attrs.flags & LIBSSH2_SFTP_ATTR_SIZE && total_recv + offsets < (off_t)attrs.filesize)
				{
					ret = -1;
				}
				::fclose(fd);
			}
		} while (0);
		while (sftp_handle && libssh2_sftp_close(sftp_handle) == LIBSSH2_ERROR_EAGAIN);
		sftp_handle = NULL;

		if (total_recv + offsets == st.st_size && unfinished_suffix.size())
		{
			ret = rename((local + unfinished_suffix).c_str(), local.c_str());
		}
		return ret;
	}
#if 1

	int ssh_t::sftp_t::sftp_shutdown()
	{
		if (NULL == _sftp_session)
		{
			return 0;
		}
		int rc = 0;
		while ((rc = libssh2_sftp_shutdown(_sftp_session)) == LIBSSH2_ERROR_EAGAIN
			&& _ssh->waitsocket() >= 0);
		_sftp_session = NULL;
		return rc;
	}
	int ssh_t::sftp_t::sftp_close_handle(LIBSSH2_SFTP_HANDLE* sftp_handle)
	{
		if (NULL == sftp_handle)
		{
			return 0;
		}
		int rc = 0;
		while ((rc = libssh2_sftp_close(sftp_handle)) == LIBSSH2_ERROR_EAGAIN);
		return rc;
	}
	LIBSSH2_SFTP_HANDLE* ssh_t::sftp_t::sftp_open(const std::string& path, long flags, long mode)
	{
		if (NULL == _sftp_session)
		{
			if (NULL == sftp_init()) return NULL;
			return NULL;
		}
		LIBSSH2_SFTP_HANDLE* sftp_handle = NULL;
		while ((sftp_handle = libssh2_sftp_open(_sftp_session, path.c_str(), flags, mode)) == NULL
			&& libssh2_session_last_errno(_ssh->_session) == LIBSSH2_ERROR_EAGAIN
			&& _ssh->waitsocket() >= 0);
		_ssh->get_lib_error();
		return sftp_handle;
	}
	int ssh_t::sftp_t::sftp_close(LIBSSH2_SFTP_HANDLE* sftp_handle)
	{
		return sftp_close_handle(sftp_handle);
	}
	int ssh_t::sftp_t::sftp_read(LIBSSH2_SFTP_HANDLE* sftp_handle, char* buf, size_t len)
	{
		int ret = 0;
		while (len)
		{
			int rn = 0;
			while ((rn = libssh2_sftp_read(sftp_handle, buf, len)) == LIBSSH2_ERROR_EAGAIN
				&& _ssh->waitsocket() >= 0);
			if (rn <= 0)
			{
				break;
			}
			buf += rn;
			len -= rn;
			ret += rn;
		}
		_ssh->get_lib_error();
		return ret;
	}
	int ssh_t::sftp_t::sftp_write(LIBSSH2_SFTP_HANDLE* sftp_handle, const char* buf, size_t len)
	{
		int ret = 0;
		while (len)
		{
			int rn = 0;
			while ((rn = libssh2_sftp_write(sftp_handle, buf, len)) == LIBSSH2_ERROR_EAGAIN
				&& _ssh->waitsocket() >= 0);
			if (rn <= 0)
			{
				break;
			}
			buf += rn;
			len -= rn;
			ret += rn;
		}
		_ssh->get_lib_error();
		return ret;
	}
	LIBSSH2_SFTP_HANDLE* ssh_t::sftp_t::sftp_opendir(const std::string& path)
	{
		if (NULL == _sftp_session)
		{
			return NULL;
		}
		LIBSSH2_SFTP_HANDLE* sftp_handle = NULL;
		while ((sftp_handle = libssh2_sftp_opendir(_sftp_session, path.c_str())) == NULL
			&& libssh2_session_last_errno(_ssh->_session) == LIBSSH2_ERROR_EAGAIN
			&& _ssh->waitsocket() >= 0);
		_ssh->get_lib_error();
		return sftp_handle;
	}
	int ssh_t::sftp_t::sftp_closedir(LIBSSH2_SFTP_HANDLE* sftp_handle)
	{
		return sftp_close_handle(sftp_handle);
	}
	int ssh_t::sftp_t::sftp_readdir(LIBSSH2_SFTP_HANDLE* sftp_handle, char* buf, size_t buflen, LIBSSH2_SFTP_ATTRIBUTES* attrs)
	{
		int rc = 0;
		while ((rc = libssh2_sftp_readdir(sftp_handle, buf, buflen, attrs)) == LIBSSH2_ERROR_EAGAIN
			&& _ssh->waitsocket() >= 0);
		return rc;
	}
	int ssh_t::sftp_t::sftp_unlink(const std::string& path)
	{
		int rc = 0;
		while ((rc = libssh2_sftp_unlink(_sftp_session, path.c_str())) == LIBSSH2_ERROR_EAGAIN
			&& _ssh->waitsocket() >= 0);
		return rc;
	}
	int ssh_t::sftp_t::sftp_mkdir(const std::string& path, long mode)
	{
		int rc = 0;
		while ((rc = libssh2_sftp_mkdir(_sftp_session, path.c_str(), mode)) == LIBSSH2_ERROR_EAGAIN
			&& _ssh->waitsocket() >= 0);
		return rc;
	}
	int ssh_t::sftp_t::sftp_rmdir(const std::string& path)
	{
		int rc = 0;
		while ((rc = libssh2_sftp_rmdir(_sftp_session, path.c_str())) == LIBSSH2_ERROR_EAGAIN
			&& _ssh->waitsocket() >= 0);
		return rc;
	}
	int ssh_t::sftp_t::sftp_stat(const std::string& path, LIBSSH2_SFTP_ATTRIBUTES* attrs)
	{
		int rc = 0;
		while ((rc = libssh2_sftp_stat(_sftp_session, path.c_str(), attrs)) == LIBSSH2_ERROR_EAGAIN
			&& _ssh->waitsocket() >= 0);
		return rc;
	}
	int ssh_t::sftp_t::sftp_lstat(const std::string& path, LIBSSH2_SFTP_ATTRIBUTES* attrs)
	{
		int rc = 0;
		while ((rc = libssh2_sftp_lstat(_sftp_session, path.c_str(), attrs)) == LIBSSH2_ERROR_EAGAIN
			&& _ssh->waitsocket() >= 0);
		return rc;
	}
	int ssh_t::sftp_t::sftp_setstat(const std::string& path, LIBSSH2_SFTP_ATTRIBUTES* attrs)
	{
		int rc = 0;
		while ((rc = libssh2_sftp_setstat(_sftp_session, path.c_str(), attrs)) == LIBSSH2_ERROR_EAGAIN
			&& _ssh->waitsocket() >= 0);
		return rc;
	}
	int ssh_t::sftp_t::sftp_readlink(const std::string& path, char* buf, size_t buflen)
	{
		int rc = 0;
		while ((rc = libssh2_sftp_readlink(_sftp_session, path.c_str(), buf, buflen)) == LIBSSH2_ERROR_EAGAIN
			&& _ssh->waitsocket() >= 0);
		return rc;
	}
	int ssh_t::sftp_t::sftp_symlink(const std::string& path, const std::string& link)
	{
		int rc = 0;
		while ((rc = libssh2_sftp_symlink(_sftp_session, path.c_str(), (char*)link.c_str())) == LIBSSH2_ERROR_EAGAIN
			&& _ssh->waitsocket() >= 0);
		return rc;
	}
	int ssh_t::sftp_t::sftp_realpath(const std::string& path, char* buf, size_t buflen)
	{
		int rc = 0;
		while ((rc = libssh2_sftp_readlink(_sftp_session, path.c_str(), buf, buflen)) == LIBSSH2_ERROR_EAGAIN
			&& _ssh->waitsocket() >= 0);
		return rc;
	}
	void ssh_t::sftp_t::init_attrs(LIBSSH2_SFTP_ATTRIBUTES* attrs, struct stat* st)
	{
		memset(attrs, 0, sizeof(*attrs));
		attrs->filesize = st->st_size;
		attrs->flags |= LIBSSH2_SFTP_ATTR_SIZE;
		attrs->permissions = st->st_mode;
		attrs->flags |= LIBSSH2_SFTP_ATTR_PERMISSIONS;
		attrs->atime = st->st_atime;
		attrs->mtime = st->st_mtime;
		attrs->flags |= LIBSSH2_SFTP_ATTR_ACMODTIME;
	}
	int ssh_t::sftp_t::sftp_copy_file(const std::string& local, const std::string& remote)
	{
		char buf[1024] = {}; size_t buflen = 1024;
		struct stat st;
		memset(&st, 0, sizeof(st));
		stat(local.c_str(), &st);
		if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode))
		{
			return -1;
		}
		LIBSSH2_SFTP_ATTRIBUTES attrs;
		init_attrs(&attrs, &st);
		int fd = ::open(local.c_str(), O_RDONLY);
		if (fd < 0)
		{
			return -1;
		}
		LIBSSH2_SFTP_HANDLE* handle = NULL;
		handle = sftp_open(remote, LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC, attrs.permissions & 0777);
		if (NULL == handle)
		{
			::close(fd);
			return -1;
		}
		int ret = 0;
		while (1)
		{
			int rn = 0, wn = 0;
			while ((rn = ::read(fd, buf, buflen)) == -1 && EAGAIN == errno);
			if (rn > 0)
			{
				wn = sftp_write(handle, buf, rn);
				if (wn != rn)
				{
					_ssh->get_lib_error();
					ret = -1;
					break;
				}
			}
			if (rn <= 0)
			{
				if (rn < 0) ret = -1;
				break;
			}
		};
		sftp_close(handle);
		::close(fd);
		sftp_setstat(remote, &attrs);
		return ret;
	}
	int ssh_t::sftp_t::sftp_copy_link(const std::string& local, const std::string& remote)
	{
		char buf[1024] = {}; size_t buflen = 1024;
#ifndef _WIN32
		if (::readlink(local.c_str(), buf, buflen) < 0)
		{
			return -1;
		}
#endif
		return sftp_symlink(remote, buf);
	}
	int ssh_t::sftp_t::sftp_copy_dir(const std::string& local, const std::string& remote)
	{
		int ret = 0;
		struct stat st;
		memset(&st, 0, sizeof(st));
		stat(local.c_str(), &st);
		if (!S_ISDIR(st.st_mode))
		{
			return -1;
		}
#ifndef _WIN32
		DIR* dir = ::opendir(local.c_str());
		if (NULL == dir)
		{
			return -1;
		}
		LIBSSH2_SFTP_ATTRIBUTES attrs;
		init_attrs(&attrs, &st);
		sftp_mkdir(remote, attrs.permissions & 0777);
		if (sftp_stat(remote, &attrs))
		{
			return -1;
		}
		struct dirent entry, * pentry;
		while (readdir_r(dir, &entry, &pentry) == 0)
		{
			if (NULL == pentry)
			{
				break;
			}
			if (strncmp(entry.d_name, ".", 1) == 0 || strncmp(entry.d_name, "..", 2) == 0)
			{
				continue;
			}
			std::string this_local = local + "/" + entry.d_name;
			std::string this_remote = remote + "/" + entry.d_name;
			sftp_copy(this_local, this_remote);
		}
		::closedir(dir);
		dir = NULL;
#endif
		return ret;
	}
	int ssh_t::sftp_t::sftp_copy(const std::string& local, const std::string& remote)
	{
		std::string local_fix = fixpath(local);
		std::string remote_fix = fixpath(remote);
		struct stat st;
		memset(&st, 0, sizeof(st));
		if (::stat(local_fix.c_str(), &st))
		{
			_ssh->get_sys_error();
			return -1;
		}
		LIBSSH2_SFTP_ATTRIBUTES attrs;
		memset(&attrs, 0, sizeof(attrs));
		if (sftp_lstat(remote_fix, &attrs) == 0 && attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS && LIBSSH2_SFTP_S_ISDIR(attrs.permissions))
		{
			remote_fix = remote_fix + "/" + basename(local_fix);
		}
		int ret = 0;
		int alloc_buf = 0;
		if (S_ISREG(st.st_mode))
		{
			ret = sftp_copy_file(local_fix, remote_fix);
		}
		else if (S_ISLNK(st.st_mode))
		{
			ret = sftp_copy_link(local_fix, remote_fix);
		}
		else if (S_ISDIR(st.st_mode))
		{
			ret = sftp_copy_dir(local_fix, remote_fix);
		}
		else
		{
			ret = -1;
		}
		return ret;
	}

	std::string ssh_t::sftp_t::basename(const std::string& path)
	{
		std::string p = fixpath(path);
		std::string::size_type pos = p.rfind('/');
		return p.substr(pos + 1);
	}
	std::string ssh_t::sftp_t::dirname(const std::string& path)
	{
		std::string p = fixpath(path);
		std::string::size_type pos = p.rfind('/');
		return p.substr(0, pos);
	}
	std::string ssh_t::sftp_t::fixpath(const std::string& path)
	{
		std::string p = path;
		std::string::size_type pos_pre = 0, pos = 0;
		while ((pos = p.find("//", pos_pre)) != std::string::npos)
		{
			p.replace(pos, 2, "/");
			pos_pre = pos;
		}
		pos = p.rfind("/");
		if (pos == p.size() - 1)
		{
			p = p.substr(0, pos);
		}
		return p;
	}
#endif // 1

	//选择配置文件、选择私钥文件
	std::string getbof()
	{
		std::string fn;
		{
			hz::browse_openfile("选择配置文件", "", "json(*.json)\t*.json\t所有文件\t*.*\t\t", nullptr
				, [&fn](const std::vector<std::string>& fns) {
					if (fns.size())
						fn = fns[0];
				});
		}
		return fn;
	}
	int main_ssh2()
	{
#ifdef _WIN32
		system("color 00");
		system("CHCP 65001");
#endif // _WIN32
		printf("\x1b]0;h\x07");

		uint32_t* cc = get_wcolor();
		for (size_t i = 0; i < 16; i++)
		{
			auto str = get_wcname(i, 0);
			printf("\x1b[01;3%dm%s\x1b[0m\n", (int)i % 8, str);
		}
		njson k = { 1,2,3 };
		njson info = hz::ssh_t::load_info("pgn.json");
		if (info.empty()) { info = hz::ssh_t::load_info(getbof()); }
		auto ssh = hz::ssh_t::new_run(info, [](hz::ssh_t* pt, std::string* d) {
			if (d) {}	// 通道返回的数据
			});
		if (ssh)
		{
			ssh->set_func_auth([](hz::ssh_t*) {
				std::string str;
				char ch = 0;
				printf("password: ");
				do
				{
					ch = getch();
					if (ch == '\r') { ch = '\n'; printf("\n"); }
					else { str.push_back(ch); printf("*"); }
				} while (ch != '\n');
				return str;
				}
				, [](hz::ssh_t*) {
					std::string prikfile;
					{
						hz::browse_openfile("选择私钥文件", "", "通用证书格式(*.pem)\t*.pem\t所有文件\t*.*\tputty(*.ppk)\t*.ppk\t", nullptr
							, [&prikfile](const std::vector<std::string>& fns) { if (fns.size()) { prikfile = fns[0]; } });
					}
					return prikfile;
				});
			ssh->run_thr(false);
			auto logindata = ssh->get_info();
			while (1)
			{
				ssh->run_cmd();
				sleep_ms(1);
			}
		}
		return 0;
	}
	/*
		//hz::save_json(logindata, "p.json");
	{
	  "idx": 0,"terms": [ "xterm-color", "xterm", "vanilla", "rxvt" ],
	  "list": [
		{
		  "host": "127.0.0.1",
		  "login_name": "root",
		  "login_type": 1,
		  "password": "",
		  "port": 22,
		  "term_idx": 0,
		  "prikfile": "",
		  "prikdata": ""
		}
	  ]
	}

	*/

}
// !hz
// win32 color
/*
{
	"name" : "Campbell",

	"cursorColor": "#FFFFFF",
	"selectionBackground": "#FFFFFF",

	"background" : "#0C0C0C",
	"foreground" : "#CCCCCC",

	"black" : "#0C0C0C",
	"blue" : "#0037DA",
	"cyan" : "#3A96DD",
	"green" : "#13A10E",
	"purple" : "#881798",
	"red" : "#C50F1F",
	"white" : "#CCCCCC",
	"yellow" : "#C19C00",
	"brightBlack" : "#767676",
	"brightRed" : "#E74856",
	"brightGreen" : "#16C60C",
	"brightYellow" : "#F9F1A5"
	"brightBlue" : "#3B78FF",
	"brightPurple" : "#B4009E",
	"brightCyan" : "#61D6D6",
	"brightWhite" : "#F2F2F2",
},
*/
// window配色方案...黑色,红色,绿色,黄色,蓝色,紫色,青色,白色{0-7}、亮{8-15}
static uint32_t wcolor[] = { 0xff0C0C0C,0xff1F0FC5,0xff0EA113,0xff009CC1,0xffDA3700,0xff981788,0xffdd963a,0xffcccccc,
0xff767676,0xff5648e7,0xff0cc616,0xffa5f1f9,0xffff783b,0xff9e00b4,0xffd6d661,0xfff2f2f2
};
uint32_t* get_wcolor()
{
	return wcolor;
}
const char* get_wcname(int x, int lang)
{
	static const void* cns[] = { u8"黑色",u8"红色",u8"绿色",u8"黄色",u8"蓝色",u8"紫色",u8"青色",u8"白色",
		u8"亮黑色",u8"亮红色",u8"亮绿色",u8"亮黄色",u8"亮蓝色",u8"亮紫色",u8"亮青色",u8"亮白色" };
	static const char* ens[] = { "Black","Red","Green","Yellow","Blue","Purple","Cyan","White","BrightBlack","brightRed","brightGreen","brightYellow","brightBlue","brightPurple","brightCyan","brightWhite" };
	return lang ? ens[x % 16] : (char*)cns[x % 16];
}
//Xterm 256 color dictionary 
// bgra
static uint32_t xc[256] = {
 0xff000000, 0xff800000, 0xff008000, 0xff808000, 0xff000080,	// 0,1,2,3,4
 0xff800080, 0xff008080, 0xffc0c0c0, 0xff808080, 0xffff0000,	// 5,6,7,8,9
 0xff00ff00, 0xffffff00, 0xff0000ff, 0xffff00ff, 0xff00ffff,
 0xffffffff, 0xff000000, 0xff00005f, 0xff000087, 0xff0000af,
 0xff0000df, 0xff0000ff, 0xff005f00, 0xff005f5f, 0xff005f87,
 0xff005faf, 0xff005fdf, 0xff005fff, 0xff008700, 0xff00875f,
 0xff008787, 0xff0087af, 0xff0087df, 0xff0087ff, 0xff00af00,
 0xff00af5f, 0xff00af87, 0xff00afaf, 0xff00afdf, 0xff00afff,
 0xff00df00, 0xff00df5f, 0xff00df87, 0xff00dfaf, 0xff00dfdf,
 0xff00dfff, 0xff00ff00, 0xff00ff5f, 0xff00ff87, 0xff00ffaf,
 0xff00ffdf, 0xff00ffff, 0xff5f0000, 0xff5f005f, 0xff5f0087,
 0xff5f00af, 0xff5f00df, 0xff5f00ff, 0xff5f5f00, 0xff5f5f5f,
 0xff5f5f87, 0xff5f5faf, 0xff5f5fdf, 0xff5f5fff, 0xff5f8700,
 0xff5f875f, 0xff5f8787, 0xff5f87af, 0xff5f87df, 0xff5f87ff,
 0xff5faf00, 0xff5faf5f, 0xff5faf87, 0xff5fafaf, 0xff5fafdf,
 0xff5fafff, 0xff5fdf00, 0xff5fdf5f, 0xff5fdf87, 0xff5fdfaf,
 0xff5fdfdf, 0xff5fdfff, 0xff5fff00, 0xff5fff5f, 0xff5fff87,
 0xff5fffaf, 0xff5fffdf, 0xff5fffff, 0xff870000, 0xff87005f,
 0xff870087, 0xff8700af, 0xff8700df, 0xff8700ff, 0xff875f00,
 0xff875f5f, 0xff875f87, 0xff875faf, 0xff875fdf, 0xff875fff,
 0xff878700, 0xff87875f, 0xff878787, 0xff8787af, 0xff8787df,
 0xff8787ff, 0xff87af00, 0xff87af5f, 0xff87af87, 0xff87afaf,
 0xff87afdf, 0xff87afff, 0xff87df00, 0xff87df5f, 0xff87df87,
 0xff87dfaf, 0xff87dfdf, 0xff87dfff, 0xff87ff00, 0xff87ff5f,
 0xff87ff87, 0xff87ffaf, 0xff87ffdf, 0xff87ffff, 0xffaf0000,
 0xffaf005f, 0xffaf0087, 0xffaf00af, 0xffaf00df, 0xffaf00ff,
 0xffaf5f00, 0xffaf5f5f, 0xffaf5f87, 0xffaf5faf, 0xffaf5fdf,
 0xffaf5fff, 0xffaf8700, 0xffaf875f, 0xffaf8787, 0xffaf87af,
 0xffaf87df, 0xffaf87ff, 0xffafaf00, 0xffafaf5f, 0xffafaf87,
 0xffafafaf, 0xffafafdf, 0xffafafff, 0xffafdf00, 0xffafdf5f,
 0xffafdf87, 0xffafdfaf, 0xffafdfdf, 0xffafdfff, 0xffafff00,
 0xffafff5f, 0xffafff87, 0xffafffaf, 0xffafffdf, 0xffafffff,
 0xffdf0000, 0xffdf005f, 0xffdf0087, 0xffdf00af, 0xffdf00df,
 0xffdf00ff, 0xffdf5f00, 0xffdf5f5f, 0xffdf5f87, 0xffdf5faf,
 0xffdf5fdf, 0xffdf5fff, 0xffdf8700, 0xffdf875f, 0xffdf8787,
 0xffdf87af, 0xffdf87df, 0xffdf87ff, 0xffdfaf00, 0xffdfaf5f,
 0xffdfaf87, 0xffdfafaf, 0xffdfafdf, 0xffdfafff, 0xffdfdf00,
 0xffdfdf5f, 0xffdfdf87, 0xffdfdfaf, 0xffdfdfdf, 0xffdfdfff,
 0xffdfff00, 0xffdfff5f, 0xffdfff87, 0xffdfffaf, 0xffdfffdf,
 0xffdfffff, 0xffff0000, 0xffff005f, 0xffff0087, 0xffff00af,
 0xffff00df, 0xffff00ff, 0xffff5f00, 0xffff5f5f, 0xffff5f87,
 0xffff5faf, 0xffff5fdf, 0xffff5fff, 0xffff8700, 0xffff875f,
 0xffff8787, 0xffff87af, 0xffff87df, 0xffff87ff, 0xffffaf00,
 0xffffaf5f, 0xffffaf87, 0xffffafaf, 0xffffafdf, 0xffffafff,
 0xffffdf00, 0xffffdf5f, 0xffffdf87, 0xffffdfaf, 0xffffdfdf,
 0xffffdfff, 0xffffff00, 0xffffff5f, 0xffffff87, 0xffffffaf,
 0xffffffdf, 0xffffffff, 0xff080808, 0xff121212, 0xff1c1c1c,
 0xff262626, 0xff303030, 0xff3a3a3a, 0xff444444, 0xff4e4e4e,
 0xff585858, 0xff606060, 0xff666666, 0xff767676, 0xff808080,
 0xff8a8a8a, 0xff949494, 0xff9e9e9e, 0xffa8a8a8, 0xffb2b2b2,
 0xffbcbcbc, 0xffc6c6c6, 0xffd0d0d0, 0xffdadada, 0xffe4e4e4,
 0xffeeeeee };
