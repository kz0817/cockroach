#include <sstream>
#include <sys/types.h> 
#include <unistd.h>
#include <sys/mman.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <stdint.h>
#include "utils.h"
#include "shm_param_note.h"

const int shm_param_note::LEN_RECIPE_FILE_PATH_SZ = 2;

// --------------------------------------------------------------------------
// private functions
// --------------------------------------------------------------------------
string shm_param_note::get_shm_name(void)
{
	stringstream ss;
	ss << "cockroach-shm-param-note-";
	ss << getpid();
	return ss.str();
}

bool shm_param_note::shm_write(int fd, const void *buf, size_t count)
{
	int remain_count = count;
	uint8_t *ptr = (uint8_t *)buf;
	while (remain_count >= 0) {
		int written_bytes = write(fd, ptr, remain_count);
		if (written_bytes == -1) {
			if (errno == EINTR)
				continue;
			ROACH_ERR("Failed to write: %zd, %s\n",
			          remain_count,  strerror(errno));
			return false;
		} else if (written_bytes == 0) {
			ROACH_ERR("write reteurned 0\n");
			return false;
		}
		ptr += written_bytes;
		remain_count -= written_bytes;
	}
	return true;
}

bool shm_param_note::shm_read(int fd, void *buf, size_t count)
{
	int remain_count = count;
	uint8_t *ptr = (uint8_t *)buf;
	while (remain_count >= 0) {
		int read_bytes = write(fd, ptr, remain_count);
		if (read_bytes == -1) {
			if (errno == EINTR)
				continue;
			ROACH_ERR("Failed to read: %zd, %s\n",
			          remain_count,  strerror(errno));
			return false;
		} else if (read_bytes == 0) {
			ROACH_ERR("read reteurned 0\n");
			return false;
		}
		ptr += read_bytes;
		remain_count -= read_bytes;
	}
	return true;
}

// --------------------------------------------------------------------------
// public functions
// --------------------------------------------------------------------------
shm_param_note::shm_param_note(void)
: m_has_error(true)
{
}

shm_param_note::~shm_param_note(void)
{
}

void shm_param_note::open(void)
{
	m_has_error = true;
	string shm_name = get_shm_name();
	int fd = shm_open(shm_name.c_str(), O_RDONLY, 0644);
	if (fd == -1) {
		ROACH_ERR("Failed to call shm_open: %s\n", strerror(errno));
		return;
	}

	// size [2B]
	uint16_t recipe_path_len;
	if (!shm_read(fd, &recipe_path_len, LEN_RECIPE_FILE_PATH_SZ)) {
		close(fd);
		shm_unlink(shm_name.c_str());
		return;
	}

	// recipe file name
	char recipe_path[recipe_path_len+1];
	if (!shm_read(fd, recipe_path, recipe_path_len)) {
		close(fd);
		shm_unlink(shm_name.c_str());
		return;
	}
	recipe_path[recipe_path_len] = '\0';
	m_recipe_file_path = recipe_path;

	close(fd);
	m_has_error = false;
}

bool shm_param_note::create(const string &recipe_file_path)
{
	string shm_name = get_shm_name();
	int fd = shm_open(shm_name.c_str(), O_RDWR|O_CREAT|O_TRUNC, 0644);
	if (fd == -1) {
		ROACH_ERR("Failed to call shm_open: %s\n", strerror(errno));
		return false;
	}

	// calculate total size and truncate the shm
	size_t total_len = LEN_RECIPE_FILE_PATH_SZ + recipe_file_path.size();
retry_trunc:
	if (ftruncate(fd, total_len) == -1) {
		if (errno == EINTR)
			goto retry_trunc;
		ROACH_ERR("Failed to ftruncate: %zd: %s\n",
		          total_len,  strerror(errno));
		close(fd);
		shm_unlink(shm_name.c_str());
		return false;
	}

	// size [2B]
	uint16_t recipe_path_len = recipe_file_path.size();
	if (!shm_write(fd, &recipe_path_len, LEN_RECIPE_FILE_PATH_SZ)) {
		close(fd);
		shm_unlink(shm_name.c_str());
		return false;
	}

	// recipe file name
	if (!shm_write(fd, recipe_file_path.c_str(), recipe_path_len)) {
		close(fd);
		shm_unlink(shm_name.c_str());
		return false;
	}

	close(fd);
	return true;
}

bool shm_param_note::has_error(void) const
{
	return m_has_error;
}

const string &shm_param_note::get_recipe_file_path(void) const
{
	return m_recipe_file_path;
}
