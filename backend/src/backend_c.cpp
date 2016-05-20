/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <string>
#include <vector>
#include <string.h>

#include <backend.hpp>
#include <backendexception.hpp>
#include <options.hpp>
#include <tdisk.hpp>

extern "C" {
	#include <backend.h>
}

using std::string;
using std::vector;

/**
  * This macro can be used to create a function
  * implementation of a C backend function which
  * simply calls the C++ equivalent
 **/
#define C_FUNCTION_IMPLEMENTATION(name) \
struct BackendResult* name(int argc, char *args[], struct Options *options) \
{ \
	td::BackendResult *r = new td::BackendResult(); \
	\
	try { \
		td::Options *o = (td::Options*)options; \
		*r = td::name(vector<string>(args, args+argc), *o); \
	} catch (const td::BackendException &e) { \
		r->error(td::BackendResultType::general, e.what()); \
	} catch (const td::tDiskException &e) { \
		r->error(td::BackendResultType::general, e.what()); \
	} catch (...) { \
		r->error(td::BackendResultType::general, "Unknown exception thrown"); \
	} \
	\
	return (struct BackendResult*)r; \
}

C_FUNCTION_IMPLEMENTATION(get_tDisks)
C_FUNCTION_IMPLEMENTATION(get_tDisk)
C_FUNCTION_IMPLEMENTATION(get_devices)
C_FUNCTION_IMPLEMENTATION(create_tDisk)
C_FUNCTION_IMPLEMENTATION(add_tDisk)
C_FUNCTION_IMPLEMENTATION(remove_tDisk)
C_FUNCTION_IMPLEMENTATION(get_max_sectors)
C_FUNCTION_IMPLEMENTATION(get_size_bytes)
C_FUNCTION_IMPLEMENTATION(get_sector_index)
C_FUNCTION_IMPLEMENTATION(get_all_sector_indices)
C_FUNCTION_IMPLEMENTATION(clear_access_count)
C_FUNCTION_IMPLEMENTATION(get_internal_devices_count)
C_FUNCTION_IMPLEMENTATION(get_device_info)
C_FUNCTION_IMPLEMENTATION(load_config_file)
C_FUNCTION_IMPLEMENTATION(get_device_advice)
C_FUNCTION_IMPLEMENTATION(get_files_at)
C_FUNCTION_IMPLEMENTATION(get_files_on_disk)
C_FUNCTION_IMPLEMENTATION(tdisk_post_create)
C_FUNCTION_IMPLEMENTATION(performance_improvement)

Options* create_options()
{
	return (struct Options*)new td::Options();
}

Options* create_default_options()
{
	return (struct Options*)new td::Options(td::getDefaultOptions());
}

void free_options(Options *o)
{
	delete (td::Options*)o;
}

void BackendResult_delete(struct BackendResult *r)
{
	delete (td::BackendResult*)r;
}

const char* BackendResult_result(struct BackendResult *r)
{
	return ((td::BackendResult*)r)->result().c_str();
}

void BackendResult_messages(struct BackendResult *r, char *out, size_t length)
{
	strncpy(out, ((td::BackendResult*)r)->messages().c_str(), length);
}

void BackendResult_warnings(struct BackendResult *r, char *out, size_t length)
{
	strncpy(out, ((td::BackendResult*)r)->warnings().c_str(), length);
}

void BackendResult_errors(struct BackendResult *r, char *out, size_t length)
{
	strncpy(out, ((td::BackendResult*)r)->errors().c_str(), length);
}
