/*
	adding DDS loading support to stbi
*/

#ifndef HEADER_STB_IMAGE_DDS_AUGMENTATION
#define HEADER_STB_IMAGE_DDS_AUGMENTATION

#ifdef __cplusplus
extern "C" {
#endif
	//	is it a DDS file?
	extern int      stbi_dds_test_memory(stbi_uc const* buffer, int len);
	extern int      stbi_dds_free_memory(stbi_uc* d);

	typedef struct stbi_info_t {
		int width;
		int height;
		int depth;
		int arraySize;
		int mipMapCount;
		int comp;
		stbi_uc* data;
		int size;
	}stbi_info_t;

	extern stbi_uc* stbi_dds_load(char* filename, int req_comp, stbi_info_t* p);
	extern stbi_uc* stbi_dds_load_from_memory(stbi_uc const* buffer, int len, int req_comp, stbi_info_t* p);
#ifndef STBI_NO_STDIO
	extern int      stbi_dds_test_file(FILE* f);
	extern stbi_uc* stbi_dds_load_from_file(FILE* f, int req_comp, stbi_info_t* p);
#endif

#ifdef __cplusplus
}
#endif
//
//
////   end header file   /////////////////////////////////////////////////////
#endif // HEADER_STB_IMAGE_DDS_AUGMENTATION
