/**
  *
  * tDisk backend
  * @author Thomas Sparber (2015-2016)
  *
 **/

#include <tdisk.hpp>

using std::make_pair;
using std::ostream;
using std::pair;
using std::sort;
using std::string;
using std::vector;

using namespace td;

/**
  * Handles the error of the C-functions and throws the
  * corresponding tDiskException
  * @param ret The return code of the C function
 **/
static void handleError(int ret)
{
	if(ret >= 0)return;

	switch(-ret)
	{
	case ENODEV:
		throw tDiskOfflineException("The driver is offline");
	default:
		throw tDiskException(strerror(errno));
	}
}

void tDisk::getTDisks(vector<string> &out)
{
	vector<char*> buffer(100);
	for(char* &c : buffer)c = new char[256];

	int devices = c::tdisk_get_devices(&buffer[0], 256, 100);
	if(devices >= 0)
	{
		for(int i = 0; i < devices && i < (int)buffer.size(); ++i)
			out.push_back(buffer[i]);
	}

	for(char* &c : buffer)delete [] c;

	if(devices == -1)throw tDiskException("Unable to read /dev folder");
	if(devices == -2)throw tDiskException("More than 100 tDisks present");
}

void tDisk::getTDisks(vector<tDisk> &out)
{
	string error;
	string offline;
	vector<char*> buffer(100);
	for(char* &c : buffer)c = new char[256];

	int devices = c::tdisk_get_devices(&buffer[0], 256, 100);
	if(devices >= 0)
	{
		for(int i = 0; i < devices && i < (int)buffer.size(); ++i)
		{
			try {
				tDisk loaded(utils::concat("/dev/", &buffer[i][0]));
				loaded.loadSize();
				out.push_back(std::move(loaded));
			} catch (const tDiskOfflineException &e) {
				offline = utils::concat("Unable to load disk ", &buffer[i][0], ": ", e.what());
				break;
			} catch (const tDiskException &e) {
				error = utils::concat("Unable to load disk ", &buffer[i][0], ": ", e.what());
				break;
			}
		}
	}

	for(char* &c : buffer)delete [] c;

	if(error != "")throw tDiskException("Unable to load all tDisks: ", error);
	if(offline != "")throw tDiskOfflineException("Unable to load all tDisks: ", offline);
	if(devices == -1)throw tDiskException("Unable to read /dev folder");
	if(devices == -2)throw tDiskException("More than 100 tDisks present");
}

int tDisk::getMinorNumber(const string &name)
{
	if(name.length() < 7 || name.substr(0, 7) != "/dev/td")
		throw tDiskException("Invalid tDisk path ", name);

	int number;
	if(!utils::convertTo(name.substr(7), number))
		throw tDiskException("Invalid tDisk ", name);

	return number;
}

tDisk tDisk::create(int i_minornumber, unsigned int blocksize)
{
	char temp[256];
	int ret = c::tdisk_add_specific(temp, i_minornumber, blocksize);
	try {
		handleError(ret);
	} catch(const tDiskOfflineException &e) {
		throw tDiskOfflineException("Unable to create new tDisk ", i_minornumber, ": ", e.what());
	} catch(const tDiskException &e) {
		throw tDiskException("Unable to create new tDisk ", i_minornumber, ": ", e.what());
	}

	if(ret != i_minornumber)
		throw tDiskException("tDisk was created but not the desired one. Don't know why... Requested: ", i_minornumber, ", Created: ", ret);

	return tDisk(i_minornumber, temp);
}

tDisk tDisk::create(unsigned int blocksize)
{
	char temp[256];
	int ret = c::tdisk_add(temp, blocksize);
	try {
		handleError(ret);
	} catch(const tDiskOfflineException &e) {
		throw tDiskOfflineException("Unable to create new tDisk: ", e.what());
	} catch(const tDiskException &e) {
		throw tDiskException("Unable to create new tDisk: ", e.what());
	}

	return tDisk(ret, temp);
}

void tDisk::remove(int i_minornumber)
{
	int ret = c::tdisk_remove(i_minornumber);

	try {
		handleError(ret);
	} catch(const tDiskOfflineException &e) {
		throw tDiskOfflineException("Unable to remove tDisk: ", e.what());
	} catch(const tDiskException &e) {
		throw tDiskException("Unable to remove tDisk: ", e.what());
	}
}

void tDisk::remove(const string &name)
{
	int number = tDisk::getMinorNumber(name);
	int ret = c::tdisk_remove(number);

	try {
		handleError(ret);
	} catch(const tDiskOfflineException &e) {
		throw tDiskOfflineException("Unable to remove tDisk: ", e.what());
	} catch(const tDiskException &e) {
		throw tDiskException("Unable to remove tDisk: ", e.what());
	}
}

bool tDisk::checkOnline() const
{
	const string deviceName = utils::concat("td", minornumber);
	vector<string> disks;
	tDisk::getTDisks(disks);

	online = false;
	for(const string &disk : disks)
	{
		if(disk == deviceName)
		{
			online = true;
			break;
		}
	}

	return online;
}

void tDisk::addDisk(const string &path, bool format)
{
	int ret = c::tdisk_add_disk(name.c_str(), path.c_str(), format);

	try {
		handleError(ret);
	} catch (const tDiskOfflineException &e) {
		throw tDiskOfflineException("Can't add disk \"", path ,"\" to tDisk ", name, ": ", e.what());
	} catch (const tDiskException &e) {
		throw tDiskException("Can't add disk \"", path ,"\" to tDisk ", name, ": ", e.what());
	}
	online = true;
}

unsigned long long tDisk::getMaxSectors() const
{
	uint64_t maxSectors;
	int ret = c::tdisk_get_max_sectors(name.c_str(), &maxSectors);

	try {
		handleError(ret);
	} catch (const tDiskOfflineException &e) {
		throw tDiskOfflineException("Can't get max sectors for tDisk ", name, ": ", e.what());
	} catch (const tDiskException &e) {
		throw tDiskException("Can't get max sectors for tDisk ", name, ": ", e.what());
	}

	online = true;
	return maxSectors;
}

unsigned long long tDisk::getSize() const
{
	uint64_t sizeBytes;
	int ret = c::tdisk_get_size_bytes(name.c_str(), &sizeBytes);

	try {
		handleError(ret);
	} catch (const tDiskOfflineException &e) {
		throw tDiskOfflineException("Can't get size for tDisk ", name, ": ", e.what());
	} catch (const tDiskException &e) {
		throw tDiskException("Can't get size for tDisk ", name, ": ", e.what());
	}

	online = true;
	return sizeBytes;
}

unsigned int tDisk::getBlocksize() const
{
	uint32_t blocksize;
	int ret = c::tdisk_get_blocksize(name.c_str(), &blocksize);

	try {
		handleError(ret);
	} catch (const tDiskOfflineException &e) {
		throw tDiskOfflineException("Can't get blocksize for tDisk ", name, ": ", e.what());
	} catch (const tDiskException &e) {
		throw tDiskException("Can't get blocksize for tDisk ", name, ": ", e.what());
	}

	online = true;
	return blocksize;
}

f_sector_index tDisk::getSectorIndex(unsigned long long logicalSector) const
{
	f_sector_index index;
	int ret = c::tdisk_get_sector_index(name.c_str(), logicalSector, &index);

	try {
		handleError(ret);
	} catch (const tDiskOfflineException &e) {
		throw tDiskOfflineException("Can't get sector index ", logicalSector, " for tDisk ", name, ": ", e.what());
	} catch (const tDiskException &e) {
		throw tDiskException("Can't get sector index ", logicalSector, " for tDisk ", name, ": ", e.what());
	}

	online = true;
	return std::move(index);
}

vector<f_sector_info> tDisk::getAllSectorIndices() const
{
	unsigned long long max_sectors = getMaxSectors();

	performance::start("getAllSectorIndices");
	vector<f_sector_info> indices((std::size_t)max_sectors);
	int ret = c::tdisk_get_all_sector_indices(name.c_str(), &indices[0], max_sectors);
	performance::stop("getAllSectorIndices");

	try {
		handleError(ret);
	} catch (const tDiskOfflineException &e) {
		throw tDiskOfflineException("Can't all sector indices for tDisk ", name, ": ", e.what());
	} catch (const tDiskException &e) {
		throw tDiskException("Can't all sector indices for tDisk ", name, ": ", e.what());
	}

	online = true;
	return std::move(indices);
}

void tDisk::clearAccessCount()
{
	int ret = c::tdisk_clear_access_count(name.c_str());

	try {
		handleError(ret);
	} catch (const tDiskOfflineException &e) {
		throw tDiskOfflineException("Can't clear access count for tDisk ", name, ": ", e.what());
	} catch (const tDiskException &e) {
		throw tDiskException("Can't clear access count for tDisk ", name, ": ", e.what());
	}

	online = true;
}

unsigned int tDisk::getInternalDevicesCount() const
{
	unsigned int devices;
	int ret = c::tdisk_get_internal_devices_count(name.c_str(), &devices);

	try {
		handleError(ret);
	} catch (const tDiskOfflineException &e) {
		throw tDiskOfflineException("Can't get internal devices count for tDisk ", name, ": ", e.what());
	} catch (const tDiskException &e) {
		throw tDiskException("Can't get internal devices count for tDisk ", name, ": ", e.what());
	}

	online = true;
	return devices;
}

f_internal_device_info tDisk::getDeviceInfo(unsigned int device) const
{
	f_internal_device_info info;
	int ret = tdisk_get_device_info(name.c_str(), device, &info);

	try {
		handleError(ret);
	} catch (const tDiskOfflineException &e) {
		throw tDiskOfflineException("Can't get device info for device ", (int)device, " for tDisk ", name, ": ", e.what());
	} catch (const tDiskException &e) {
		throw tDiskException("Can't get device info for device ", (int)device, " for tDisk ", name, ": ", e.what());
	}

	online = true;
	return info;
}

vector<FileAssignment> tDisk::getFilesOnDevice(unsigned int device, bool getPercentages, bool filesOnly)
{
	vector<f_sector_info> sectorIndices = getAllSectorIndices();

	unsigned int blocksize = getBlocksize();
	vector<pair<unsigned long long,unsigned long long> > positions;
	for(const f_sector_info &info : sectorIndices)
	{
		if(info.physical_sector.disk == device)
			positions.push_back(make_pair(info.logical_sector*blocksize, (info.logical_sector+1)*blocksize));
	}

	vector<FileAssignment> files = fs::getFilesOnDisk(getPath(), positions, getPercentages, filesOnly);

	performance::start("sortFiles");
	sort(files.begin(), files.end(), [](const FileAssignment &a, const FileAssignment &b)
	{
		return a.percentage > b.percentage;
	});
	performance::stop("sortFiles");

	return std::move(files);
}

tDiskPerformanceImprovement tDisk::getPerformanceImprovement(std::size_t amountFiles)
{
	unsigned int fastestDevice = 1;
	double bestPerformance = 999999999;
	long double avgPerformance = 0;
	const unsigned int devices = getInternalDevicesCount();
	for(unsigned int device = 1; device <= devices; ++device)
	{
		f_internal_device_info info = getDeviceInfo(device);

		double avg_read_dec = (double)info.performance.mod_avg_read / (1 << c::get_measure_records_shift());
		double avg_write_dec = (double)info.performance.mod_avg_write / (1 << c::get_measure_records_shift());

		double avg_read_time_cycles = (double)info.performance.avg_read_time_cycles			+ avg_read_dec;
		double avg_write_time_cycles = (double)info.performance.avg_write_time_cycles		+ avg_write_dec;

		double performance = avg_read_time_cycles + avg_write_time_cycles;
		if(performance < bestPerformance)
		{
			bestPerformance = performance;
			fastestDevice = device;
		}

		avgPerformance += performance;
	}
	avgPerformance /= devices;

	vector<FileAssignment> files = getFilesOnDevice(fastestDevice, true, true);

	long double avgPercentage = 0;
	for(const FileAssignment &f : files)
	{
		avgPercentage += f.percentage;
	}
	avgPercentage /= files.size();

	long double performanceDifference = (avgPerformance - bestPerformance) / avgPerformance;

	tDiskPerformanceImprovement p((double)(performanceDifference * avgPercentage));
	for(std::size_t i = 0; i < amountFiles && i < files.size(); ++i)
	{
		const FileAssignment &f = files[i];
		p.addFile(f.filename, (double)(f.percentage * performanceDifference));
	}

	return std::move(p);
}

template <> void td::createResultString(ostream &ss, const f_sector_index &index, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	bool used = index.used;

	if(outputFormat == "json")
	{
		ss<<"{\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, index, disk, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, index, sector, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_NONMEMBER_JSON(ss, used, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, index, access_count, hierarchy+1, outputFormat); ss<<"\n";
		insertTab(ss, hierarchy); ss<<"}";
	}
	else if(outputFormat == "text")
	{
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, index, disk, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, index, sector, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_NONMEMBER_TEXT(ss, used, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, index, access_count, hierarchy+1, outputFormat); ss<<"\n";
	}
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

template <> void td::createResultString(ostream &ss, const f_sector_info &info, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	if(outputFormat == "json")
	{
		ss<<"{\n";
			insertTab(ss, hierarchy+1), CREATE_RESULT_STRING_MEMBER_JSON(ss, info, logical_sector, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1), CREATE_RESULT_STRING_MEMBER_JSON(ss, info, access_sorted_index, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1), CREATE_RESULT_STRING_MEMBER_JSON(ss, info, physical_sector, hierarchy+1, outputFormat); ss<<"\n";
		insertTab(ss, hierarchy); ss<<"}";
	}
	else if(outputFormat == "text")
	{
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, info, logical_sector, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, info, access_sorted_index, hierarchy+1, outputFormat); ss<<"\n";
		createResultString(ss, info.physical_sector, hierarchy+1, outputFormat); ss<<"\n";
	}
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

template <> void td::createResultString(ostream &ss, const f_device_performance &perf, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	double avg_read_dec		= (double)perf.mod_avg_read / (1 << c::get_measure_records_shift());
	double stdev_read_dec	= (double)perf.mod_stdev_write / (1 << c::get_measure_records_shift());
	double avg_write_dec	= (double)perf.mod_avg_write / (1 << c::get_measure_records_shift());
	double stdev_write_dec	= (double)perf.mod_stdev_write / (1 << c::get_measure_records_shift());

	double avg_read_time_cycles = (double)perf.avg_read_time_cycles			+ avg_read_dec;
	double stdev_read_time_cycles = (double)perf.stdev_read_time_cycles		+ stdev_read_dec;
	double avg_write_time_cycles = (double)perf.avg_write_time_cycles		+ avg_write_dec;
	double stdev_write_time_cycles = (double)perf.stdev_write_time_cycles	+ stdev_write_dec;

	if(outputFormat == "json")
	{
		ss<<"{\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_NONMEMBER_JSON(ss, avg_read_time_cycles, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_NONMEMBER_JSON(ss, stdev_read_time_cycles, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_NONMEMBER_JSON(ss, avg_write_time_cycles, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_NONMEMBER_JSON(ss, stdev_write_time_cycles, hierarchy+1, outputFormat); ss<<"\n";
		insertTab(ss, hierarchy); ss<<"}";
	}
	else if(outputFormat == "text")
	{
		CREATE_RESULT_STRING_NONMEMBER_TEXT(ss, avg_read_time_cycles, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_NONMEMBER_TEXT(ss, stdev_read_time_cycles, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_NONMEMBER_TEXT(ss, avg_write_time_cycles, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_NONMEMBER_TEXT(ss, stdev_write_time_cycles, hierarchy+1, outputFormat); ss<<"\n";
	}
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

template <> void td::createResultString(ostream &ss, const enum f_internal_device_type &type, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	switch(type)
	{
	case c::f_internal_device_type_file:
		createResultString(ss, "file", hierarchy, outputFormat);
		break;
	case c::f_internal_device_type_plugin:
		createResultString(ss, "plugin", hierarchy, outputFormat);
		break;
	default:
		throw FormatException("Undefined enum value ", type, " for f_internal_device_type");
	}
}

template <> void td::createResultString(ostream &ss, const f_internal_device_info &info, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	if(outputFormat == "json")
	{
		ss<<"{\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, info, disk, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, info, name, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, info, path, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, info, size, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, info, type, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, info, performance, hierarchy+1, outputFormat); ss<<"\n";
		insertTab(ss, hierarchy); ss<<"}";
	}
	else if(outputFormat == "text")
	{
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, info, disk, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, info, name, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, info, path, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, info, size, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, info, type, hierarchy+1, outputFormat); ss<<"\n";
		createResultString(ss, info.performance, hierarchy+1, outputFormat); ss<<"\n";
	}
	else
		throw FormatException("Invalid output-format ", outputFormat);
}

template <> void td::createResultString(ostream &ss, const tDisk &disk, unsigned int hierarchy, const utils::ci_string &outputFormat)
{
	if(outputFormat == "json")
	{
		ss<<"{\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, disk, minornumber, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, disk, name, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, disk, size, hierarchy+1, outputFormat); ss<<",\n";
			insertTab(ss, hierarchy+1); CREATE_RESULT_STRING_MEMBER_JSON(ss, disk, online, hierarchy+1, outputFormat); ss<<"\n";
		insertTab(ss, hierarchy); ss<<"}";
	}
	else if(outputFormat == "text")
	{
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, disk, minornumber, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, disk, name, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, disk, size, hierarchy+1, outputFormat); ss<<"\n";
		CREATE_RESULT_STRING_MEMBER_TEXT(ss, disk, online, hierarchy+1, outputFormat); ss<<"\n";
	}
	else
		throw FormatException("Invalid output-format ", outputFormat);
}
