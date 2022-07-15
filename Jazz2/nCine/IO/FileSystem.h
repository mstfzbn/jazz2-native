#pragma once

#include <string>

#ifdef _WIN32
typedef void* HANDLE;
#elif defined(__APPLE__)
#include <dirent.h>
#elif defined(__ANDROID__)
using DIR = struct DIR;
using AAssetDir = struct AAssetDir;
#else
struct __dirstream;
using DIR = struct __dirstream;
#endif

namespace nCine
{
	/// File system related methods
	class FileSystem
	{
	public:
		/// Maximum allowed length for a path string
		static const unsigned int MaxPathLength;

		/// The available permissions to check or set
		enum Permission
		{
			READ = 1,
			WRITE = 2,
			EXECUTE = 4
		};

		struct FileDate
		{
			int year;
			int month;
			int day;
			int weekDay;
			int hour;
			int minute;
			int second;
		};

		/// The class that handles directory traversal
		class Directory
		{
		public:
			Directory(const char* path);
			~Directory();

			/// Opens a directory for traversal
			bool open(const char* path);
			/// Closes an opened directory
			void close();
			/// Returns the name of the next file inside the directory or `nullptr`
			const char* readNext();

		private:
#ifdef _WIN32
			bool firstFile_ = true;
			HANDLE hFindFile_ = NULL;
			char fileName_[260];
#else
#ifdef __ANDROID__
			AAssetDir* assetDir_ = nullptr;
#endif
			DIR* dirStream_ = nullptr;
#endif
		};

		/// Joins together two path components
		static std::string joinPath(const std::string& first, const std::string& second);
		/// Returns the aboslute path after joining together two path components
		static std::string absoluteJoinPath(const std::string& first, const std::string& second);

		/// Returns the path up to, but not including, the final separator
		static std::string dirName(const char* path);
		/// Returns the path component after the final separator
		static std::string baseName(const char* path);
		/// Returns an absolute path from a relative one
		/** Also resolves dot references to current and parent directory and double separators */
		static std::string absolutePath(const char* path);

		/// Returns the extension position in the string or `nullptr` if it is not found
		static const char* extension(const char* path);
		/// Returns true if the file at `path` as the specified extension (case-insensitive comparison)
		static bool hasExtension(const char* path, const char* extension);
		/// Returns true if the path has been changed to fix the extension
		static bool fixExtension(std::string& path, const char* extension);

		/// Returns a bitmask representing the currently available disk drives on Windows
		static unsigned long logicalDrives();
		/// Returns a buffer with strings that specify valid drives in the system on Windows
		static const char* logicalDriveStrings();

		/// Returns the path of current directory
		static std::string currentDir();
		/// Sets the current working directory, the starting point for interpreting relative paths
		static bool setCurrentDir(const char* path);
		/// Returns the path of the user home directory
		static std::string homeDir();
#ifdef __ANDROID__
		/// Returns the path of the Android external storage directory
		static std::string externalStorageDir();
#endif

		/// Returns true if the specified path is a directory
		static bool isDirectory(const char* path);
		/// Returns true if the specified path is a file
		static bool isFile(const char* path);

		/// Returns true if the file or directory exists
		static bool exists(const char* path);
		/// Returns true if the file or directory is readable
		static bool isReadable(const char* path);
		/// Returns true if the file or directory is writeable
		static bool isWritable(const char* path);
		/// Returns true if the file or directory is executable
		static bool isExecutable(const char* path);

		/// Returns true if the path is a file and is readable
		static bool isReadableFile(const char* path);
		/// Returns true if the path is a file and is writeable
		static bool isWritableFile(const char* path);

		/// Returns true if the file or directory is hidden
		static bool isHidden(const char* path);
		/// Makes a file or directory hidden or not
		static bool setHidden(const char* path, bool hidden);

		/// Creates a new directory
		static bool createDir(const char* path);
		/// Deletes an empty directory
		static bool deleteEmptyDir(const char* path);
		/// Deletes a file
		static bool deleteFile(const char* path);
		/// Renames or moves a file or a directory
		static bool rename(const char* oldPath, const char* newPath);
		/// Copies a file
		static bool copy(const char* oldPath, const char* newPath);

		/// Returns the file size in bytes
		static long int fileSize(const char* path);
		/// Returns the last time the file or directory was modified
		static FileDate lastModificationTime(const char* path);
		/// Returns the last time the file or directory was accessed
		static FileDate lastAccessTime(const char* path);

		/// Returns the file or directory permissions in a mask
		static int permissions(const char* path);
		/// Sets the file or directory permissions to those of the mask
		static bool changePermissions(const char* path, int mode);
		/// Adds the permissions in the mask to a file or a directory
		static bool addPermissions(const char* path, int mode);
		/// Removes the permissions in the mask from a file or a directory
		static bool removePermissions(const char* path, int mode);

		/// Returns the base directory for data loading
		inline static const std::string& dataPath() {
			return dataPath_;
		}
		/// Returns the writable directory for saving data
		static const std::string& savePath();

	private:
		/// The path for the application to load files from
		static std::string dataPath_;
		/// The path for the application to write files into
		static std::string savePath_;

		/// Determines the correct save path based on the platform
		static void initSavePath();

		/// The `AppConfiguration` class needs to access `dataPath_`
		friend class AppConfiguration;
	};

	using fs = FileSystem;

}
