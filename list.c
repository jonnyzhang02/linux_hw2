#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#define MAX_PATH 512  //最大文件长度定义为512

void read_dir(char *dir, void (*func)(char *)); //读目录
void print_file_info(char *name); //打印文件信息
void analyse_parameter(int argc, char *argv[]); //分析参数
void error(); //错误处理

struct para { //0表示未使用，1表示使用
	int r; 
	int a;
	int l;
	int h;
	int m;
	int min; //文件大小最小值
	int max; //文件大小最大值
	int day; //修改天数
}Parameter = {0, 0, 0, 0, 0, -1, -1, -1}; //初始化

char path[MAX_PATH] = "."; //默认路径为当前目录


/*
对目录中所有文件执行print_result操作
把print_result函数作为参数传进去
*/
void read_dir(char *dir, void (*func)(char *)) {
	char name[MAX_PATH];
	struct dirent *dp;
	DIR *dfd;
	
	if ((dfd = opendir(dir)) == NULL) {
		fprintf(stderr, "\033[31mread_dir:无法打开%s\033[0m\n", dir); // 打开目录失败
		return; 
	}
	
	while ((dp = readdir(dfd)) != NULL) { //读目录记录项
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp -> d_name, "..") == 0) {
			continue;  //跳过当前目录以及父目录
		}
		
		if (strlen(dir) + strlen(dp -> d_name) + 2 > sizeof(name)) {
			fprintf(stderr, "\033[31mread_dir : 文件名 %s %s 太长\033[0m\n", dir, dp->d_name); //文件名过长
		}
		else {
			if (dp->d_name[0] == '.' && Parameter.a == 0) //以.开头的文件 
				continue;
			sprintf(name, "%s/%s", dir, dp->d_name); //拼接文件名
			(*func)(name); //执行函数
		}
	}
	closedir(dfd); 	// 关闭目录
}

/*打印文件信息*/
void print_file_info(char *name) {
	struct stat stbuf; //文件状态结构
	/*
	struct stat { 
     dev_t st_dev; // 文件所在设备ID 
     ino_t st_ino; // 结点(inode)编号  
     mode_t st_mode; // 保护模式 
     nlink_t st_nlink; // 硬链接个数  
     uid_t st_uid; // 所有者用户ID  
     gid_t st_gid; // 所有者组ID  
     dev_t st_rdev; // 设备ID(如果是特殊文件) 
     off_t st_size; // 总体尺寸，以字节为单位 
     blksize_t st_blksize; // 文件系统 I/O 块大小
     blkcnt_t st_blocks; // 已分配的 512B 块个数
     time_t st_atime; // 上次访问时间 
     time_t st_mtime; // 上次更新时间 
     time_t st_ctime; // 上次状态更改时间 
	};
	*/
	
	// 获取文件状态并储存在stbuf结构中
	if (stat(name, &stbuf) == -1) { //
		fprintf(stderr, "\033[31;41m打开%s 失败\033[0m\n", name); //打开文件失败
		error();
		return;
	}
	
	//判断是否是目录
	if (S_ISDIR(stbuf.st_mode)) {
		//如果是目录，根据-r打印目录size及name
		if (strcmp(name, path) != 0)
			printf("\033[35m%8ld\033[0m\t\033[36m%s/\033[0m\n", stbuf.st_size, name); //不是当前目录，打印目录名
		if (Parameter.r == 1 || strcmp(name, path) == 0) { //如果有 -r 遍历下一级目录
			read_dir(name, print_file_info);
		}
	}
	else {
		//不是目录，根据-l -h -m打印文件size及name
		int flag = 1; //判断是否要输出 
		if (Parameter.l == 1 && stbuf.st_size < Parameter.min) flag = 0; //不满足条件
		if (Parameter.h == 1 && stbuf.st_size > Parameter.max) flag = 0; 
		if (Parameter.m == 1) { 	//判断文件修改时间
			struct timeval nowTime; //当前时间
			gettimeofday(&nowTime, NULL); //获取当前时间
			if (nowTime.tv_sec - stbuf.st_mtim.tv_sec > (time_t)(Parameter.day*86400)) 	//判断是否超过指定天数
				flag = 0; //不满足条件
		}
		if (flag == 1) 
			printf("\033[35m%8ld\033[0m\t\033[32m%s\n\033[0m", stbuf.st_size, name); //满足条件，打印文件名
	}
}

/*分析参数，不具体考虑错误*/
void analyse_parameter(int argc, char *argv[]) {
	//遍历参数
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-r") == 0) {
			Parameter.r = 1; //有-r参数
		}
		else if (strcmp(argv[i], "-a") == 0) {
			Parameter.a = 1; //有-a参数
		}
		else if (strcmp(argv[i], "-l") == 0) {
			Parameter.l = 1; //有-l参数
			i++; //下一个参数是最小值
			Parameter.min = atoi(argv[i]); //转换为整数保存
		}
		else if (strcmp(argv[i], "-h") == 0) {
			Parameter.h = 1;//有-h参数
			i++; //下一个参数是最大值
			Parameter.max = atoi(argv[i]);//转换为整数保存
		}
		else if (strcmp(argv[i], "-m") == 0) {
			Parameter.m = 1;// 有-m参数
			i++; // 下一个参数是天数
			Parameter.day = atoi(argv[i]); //转换为整数保存
		}
		else if (strcmp(argv[i], "--") == 0) { //--标记参数结束
			i++; //下一个参数是路径
			if (i == argc) strcpy(path, "."); //如果没有路径，使用当前目录
			else if (i = argc - 1) strcpy(path, argv[i]); //如果有路径，保存路径
			else error(); //参数错误
			break; //参数结束
		}
		else if (i = argc - 1) {
			strcpy(path, argv[i]); //如果没有--标记，最后一个参数是路径
		}
		else {
			error(); //参数错误
		}
	}
}

void error() {
	printf("\033[45m张扬2020212185 list.c 列表普通磁盘文件,包括文件名和文件大小,默认路径为当前目录\n\n");
	printf("  -?           显示帮助信息\n");
	printf("  -a           列出文件名第一个字符为圆点的普通文件\n");
	printf("  -r           递归方式列出子目录\n");
	printf("  -l <bytes>   后跟一整数,限定文件大小的最小值\n");
	printf("  -h <bytes>   后跟一整数,限定文件大小的最大值\n");
	printf("  -m <days>    后跟一整数n，限定文件的最近修改时间必须在n天内\033[0m\n\n");
	exit(-1);
}

int main(int argc, char *argv[]) {
	analyse_parameter(argc, argv);
	printf("\033[;34m文件大小\t\033[0m\033[;34m文件名\n\033[0m");
	print_file_info(path);
	return 0;
}


