#include <string>
#include <vector>
#include <string.h>

#include <frontend.hpp>
#include <frontendexception.hpp>
#include <options.hpp>
#include <tdisk.hpp>

extern "C" {
	#include <frontend.h>
}

using std::string;
using std::vector;

#define C_FUNCTION_IMPLEMENTATION(name) \
int name(int argc, char *args[], struct Options *options, char *out, int out_length) \
{ \
	try { \
		td::Options *o = (td::Options*)options; \
		const string &result = td::name(vector<string>(args, args+argc), *o); \
		strncpy(out, result.c_str(), out_length); \
		return 0; \
	} catch (const td::FrontendException &e) { \
		strncpy(out, e.what.c_str(), out_length); \
		return -1; \
	} catch (const td::tDiskException &e) { \
		strncpy(out, e.message.c_str(), out_length); \
		return -2; \
	} catch (...) { \
		strncpy(out, "Unknown exception thrown", out_length); \
		return -3; \
	} \
}

C_FUNCTION_IMPLEMENTATION(add_tDisk)
C_FUNCTION_IMPLEMENTATION(remove_tDisk)
C_FUNCTION_IMPLEMENTATION(get_max_sectors)
C_FUNCTION_IMPLEMENTATION(get_sector_index)
C_FUNCTION_IMPLEMENTATION(get_all_sector_indices)
C_FUNCTION_IMPLEMENTATION(clear_access_count)
C_FUNCTION_IMPLEMENTATION(get_internal_devices_count)
C_FUNCTION_IMPLEMENTATION(get_device_info)
C_FUNCTION_IMPLEMENTATION(load_config_file)
C_FUNCTION_IMPLEMENTATION(get_device_advice)

