#pragma once
#include <HalStorage.h>
#include <Stream.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>

/**
 * HTTP client utility for fetching content and downloading files.
 * Streams requests through esp_http_client so large downloads do not need to
 * fit in RAM.
 */
class HttpDownloader {
 public:
  using ProgressCallback = std::function<void(size_t downloaded, size_t total)>;
  using CancelCallback = std::function<bool()>;
  // Called with each body chunk as it arrives; return false to abort. Lets a
  // streaming parser consume the response without buffering the whole body.
  using DataCallback = std::function<bool(const uint8_t* data, size_t len)>;
  // Extra request headers (name, value), for APIs that authenticate via custom
  // headers instead of HTTP Basic Auth (e.g. BookOrbit's x-auth-user/x-auth-key).
  using HeaderList = std::vector<std::pair<std::string, std::string>>;

  enum DownloadError {
    OK = 0,
    HTTP_ERROR,
    FILE_ERROR,
    ABORTED,
  };

  struct DownloadOptions {
    explicit DownloadOptions(bool preservePartial = false, bool resumePartial = false,
                             CancelCallback shouldCancel = nullptr, size_t bufferSize = 0)
        : preservePartial(preservePartial),
          resumePartial(resumePartial),
          shouldCancel(std::move(shouldCancel)),
          bufferSize(bufferSize) {}

    bool preservePartial;
    bool resumePartial;
    CancelCallback shouldCancel;
    size_t bufferSize;
    // esp_http_client's own RX buffer (0 = default). Shrink for TLS servers on
    // fragmented heaps: body bytes that arrive with the headers are cached via
    // realloc in steps of this size, and that realloc competes with the transient
    // 16KB TLS record buffer for the largest free block.
    size_t clientRxBufferSize = 0;
  };

  /**
   * Fetch text content from a URL with optional credentials.
   */
  static bool fetchUrl(const std::string& url, std::string& outContent, const std::string& username = "",
                       const std::string& password = "", const HeaderList& extraHeaders = {});

  static bool fetchUrl(const std::string& url, Stream& stream, const std::string& username = "",
                       const std::string& password = "", const HeaderList& extraHeaders = {});

  /**
   * Stream the response body to onData as it arrives, without buffering it.
   */
  static bool fetchUrl(const std::string& url, const DataCallback& onData, const std::string& username = "",
                       const std::string& password = "", const HeaderList& extraHeaders = {});

  /**
   * Download a file to the SD card with optional credentials.
   */
  static DownloadError downloadToFile(const std::string& url, const std::string& destPath,
                                      ProgressCallback progress = nullptr, bool* cancelFlag = nullptr,
                                      const std::string& username = "", const std::string& password = "",
                                      DownloadOptions options = DownloadOptions(), const HeaderList& extraHeaders = {});

  /**
   * HTTP status of the last request (final redirect hop), 0 when no response was
   * received. Diagnostic only, so callers can distinguish e.g. a 404 (wrong path or
   * server version) from a transport failure.
   */
  static int lastHttpStatus;
};
