#include "../include/utils.h"

const char* format_file_size(long long bytes) {
  static char buffer[32];

  if (bytes < 1024) {
    snprintf(buffer, sizeof(buffer), "%lld B", bytes);
  }
  else if (bytes < 1024 * 1024) {
    snprintf(buffer, sizeof(buffer), "%.2f KB", bytes / 1024.0);
  }
  else if (bytes < 1024 * 1024 * 1024) {
    snprintf(buffer, sizeof(buffer), "%.2f MB", bytes / (1024.0 * 1024.0));
  }
  else {
    snprintf(buffer, sizeof(buffer), "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
  }

  return buffer;
}

char* format_time(int seconds) {
  static char buffer[32];

  if (seconds < 60) {
    snprintf(buffer, sizeof(buffer), "%ds", seconds);
  }
  else if (seconds < 3600) {
    snprintf(buffer, sizeof(buffer), "%dm%ds", seconds / 60, seconds % 60);
  }
  else {
    snprintf(buffer, sizeof(buffer), "%dh%dm", seconds / 3600, (seconds % 3600) / 60);
  }

  return buffer;
}