#include "../include/common.h"
#include "../include/progress.h"
#include "../include/utils.h"

void update_download_progress(DownloadProgress* progress) {
  time_t current_time = time(NULL);
  time_t elapsed_time = current_time - progress->start_time;

  if (elapsed_time == 0) {
    elapsed_time = 1; // 避免除零错误
  }

  // 计算下载速度
  progress->download_speed = (double)progress->downloaded_size / elapsed_time;

  // 计算完成百分比
  double percentage = 0.0;
  if (progress->total_size > 0) {
    percentage = (double)progress->downloaded_size * 100.0 / progress->total_size;
  }

  // 估算剩余时间
  long long remaining_bytes = progress->total_size - progress->downloaded_size;
  time_t estimated_remaining_time = 0;
  if (progress->download_speed > 0 && progress->total_size > 0) {
    estimated_remaining_time = (time_t)(remaining_bytes / progress->download_speed);
  }

  // 显示进度信息
  if (progress->total_size > 0) {
    print_progress_bar(percentage, progress->downloaded_size, progress->total_size, progress->download_speed, estimated_remaining_time);
  }
  else {
    print_indeterminate_progress(progress->downloaded_size, progress->download_speed, elapsed_time);
  }
}

void print_progress_bar(double percentage, long long downloaded, long long total, double speed, time_t eta) {
  const int BAR_WIDTH = 40;
  int filled_length = (int)(percentage * BAR_WIDTH / 100.0);

  // 清除当前行并回到行首
  printf("\r\033[K");

  // 颜色定义
  const char* GREEN = "\033[32m";
  const char* BLUE = "\033[34m";
  const char* YELLOW = "\033[33m";
  const char* CYAN = "\033[36m";
  const char* RESET = "\033[0m";
  const char* BOLD = "\033[1m";

  // 显示进度条
  printf("%s[%s", CYAN, GREEN);
  for (int i = 0; i < BAR_WIDTH; i++) {
    if (i < filled_length) {
      printf("█");
    }
    else if (i == filled_length && percentage < 100.0) {
      printf("▌");
    }
    else {
      printf("░");
    }
  }
  printf("%s]%s ", CYAN, RESET);

  // 显示百分比
  if (percentage >= 100.0) {
    printf("%s%s✓ 100.00%%%s ", BOLD, GREEN, RESET);
  }
  else {
    printf("%s%6.2f%%%s ", YELLOW, percentage, RESET);
  }

  // 显示下载量
  printf("%s%s%s/%s%s%s ",
    BLUE, format_file_size(downloaded), RESET,
    BOLD, format_file_size(total), RESET);

  // 显示下载速度
  printf("%s%s/s%s ",
    CYAN, format_file_size((long long)speed), RESET);

  // 显示剩余时间
  if (eta > 0 && percentage < 100.0) {
    printf("ETA: %s%s%s", YELLOW, format_time_duration(eta), RESET);
  }
  else if (percentage >= 100.0) {
    printf("%s完成%s", GREEN, RESET);
  }
  else {
    printf("ETA: %s--%s", YELLOW, RESET);
  }

  fflush(stdout); // 立即刷新输出
}

void print_indeterminate_progress(long long downloaded, double speed, time_t elapsed) {
  // 不确定大小的进度条
  const int BAR_WIDTH = 40;
  static int animation_frame = 0;

  // 清除当前行
  printf("\r\033[K");

  // 颜色定义
  const char* BLUE = "\033[34m";
  const char* CYAN = "\033[36m";
  const char* YELLOW = "\033[33m";
  const char* RESET = "\033[0m";
  const char* BOLD = "\033[1m";

  // 动画进度条
  printf("%s[%s", CYAN, BLUE);
  for (int i = 0; i < BAR_WIDTH; i++) {
    int pos = (animation_frame + i) % (BAR_WIDTH * 2);
    if (pos < BAR_WIDTH) {
      if (abs(pos - BAR_WIDTH / 2) < 3) {
        printf("█");
      }
      else if (abs(pos - BAR_WIDTH / 2) < 6) {
        printf("▌");
      }
      else {
        printf("░");
      }
    }
    else {
      printf("░");
    }
  }
  printf("%s]%s ", CYAN, RESET);

  // 显示下载信息
  printf("%s未知大小%s ", YELLOW, RESET);
  printf("%s%s%s ", BOLD, format_file_size(downloaded), RESET);
  printf("%s%s/s%s ", CYAN, format_file_size((long long)speed), RESET);
  printf("用时: %s%s%s", YELLOW, format_time_duration(elapsed), RESET);

  animation_frame = (animation_frame + 1) % (BAR_WIDTH * 2);
  fflush(stdout);
}

void print_simple_progress(long long downloaded, long long total, double speed) {
  // 简单的文本进度显示（无颜色）
  if (total > 0) {
    double percentage = (double)downloaded * 100.0 / total;
    printf("\r进度: %.1f%% (%s/%s) 速度: %s/s",
      percentage,
      format_file_size(downloaded),
      format_file_size(total),
      format_file_size((long long)speed));
  }
  else {
    printf("\r已下载: %s 速度: %s/s",
      format_file_size(downloaded),
      format_file_size((long long)speed));
  }
  fflush(stdout);
}

const char* format_time_duration(time_t seconds) {
  static char buffer[32];

  if (seconds < 60) {
    snprintf(buffer, sizeof(buffer), "%lds", seconds);
  }
  else if (seconds < 3600) {
    int minutes = seconds / 60;
    int secs = seconds % 60;
    snprintf(buffer, sizeof(buffer), "%dm%02ds", minutes, secs);
  }
  else {
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    snprintf(buffer, sizeof(buffer), "%dh%02dm%02ds", hours, minutes, secs);
  }

  return buffer;
}

void clear_progress_line() {
  printf("\r\033[K");
  fflush(stdout);
}

void print_download_summary(DownloadProgress* progress, int success) {
  clear_progress_line();

  time_t total_time = time(NULL) - progress->start_time;
  double avg_speed = total_time > 0 ? (double)progress->downloaded_size / total_time : 0;

  const char* GREEN = "\033[32m";
  const char* RED = "\033[31m";
  const char* BLUE = "\033[34m";
  const char* RESET = "\033[0m";
  const char* BOLD = "\033[1m";

  if (success) {
    printf("%s%s✓ 下载完成!%s\n", BOLD, GREEN, RESET);
  }
  else {
    printf("%s%s✗ 下载失败!%s\n", BOLD, RED, RESET);
  }

  printf("  总计下载: %s%s%s\n", BLUE, format_file_size(progress->downloaded_size), RESET);
  printf("  用时: %s%s%s\n", BLUE, format_time_duration(total_time), RESET);
  printf("  平均速度: %s%s/s%s\n", BLUE, format_file_size((long long)avg_speed), RESET);
}