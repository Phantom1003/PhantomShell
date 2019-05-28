// 实验 3-5 shell
// 徐金焱 3160101126 信息安全
#include <stdio.h> //printf(), getchar(), perror()...
#include <stdlib.h>
//malloc(), free(), execvp()...
#include <unistd.h>
//fork(), getcwd(), chdir(), environ...
#include <string.h>
//strcpy(), strncmp()...
#include <sys/wait.h>
//wait()
#include <sys/types.h>
#include <pwd.h>
//getpwuid
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
//open(),opendir()...
#include<time.h>
//time()
#include <dirent.h> //readdir()
#define MAX_LINE 80 //最大长度
//函数
void init();
//欢迎函数
int read_command();
//读取用户输入
int is_internal_cmd();
//运行内部命令
void type_prompt();
//显示提示符
void get_cmd_args();
//筛选出参数
void check_cmd_line();
//检查特殊符号: >> > | &
void shell_cd();
//cd 命令
void shell_umask();
//umask 命令
void shell_help();
//help 命令
void shell_dir();
//dir 命令
void shell_jobs();
//jobs 命令
void shell_test();
//test 命令
void redirect_output();
//输出缓存区重定向输出
void redirect_extern();
//外部命令重定向输出
void exec_external_cmd(); //运行外部命令
void using_pipe();
//管道输出
void myshell();
//myshell 命令
//全局变量
char PWD[ MAX_LINE ];
//保存当前工作路径
char SHELL[ MAX_LINE ];
//保存 Shell 路径
char* cmd_args[ MAX_LINE /2 + 1 ]; //保存命令参数
char cmd_line[ MAX_LINE ];
//保存命令
char pipe_cmd_line[ MAX_LINE ];
//当有管道时保存下一条命令
char outputfile[ MAX_LINE ];
//输出路径
char buffer[ 10240 ];
//输出缓存区
int state;
//状态存储变量,waitpid 使用
int redirect = 0;
//重定向标识
int mypipe = 0;
//管道标识
int fd[2];
//管道
extern char **environ;
//环境变量,存储在 unistd.h 中
int my_umask = 18;
//umask 掩码
int bg = 0;
//&标识
//欢迎函数
void init(){
printf("\033[1;5;36m\t\t 欢迎使用 Phantom Shell\033[0m\n");
//标题闪烁
printf("\033[1m\t\t\t\t 作者:徐金焱 3160101126\n");
printf("Shell 支持的命令有:\n");
printf("bg、cd 、clr、dir、echo、exec 、exit、environ、fg、help\n");
printf("jobs、pwd、quit、set 、shift、stop、test、time、umask、unset\n");
printf("输入`help 名称`查看详细。\033[0m\n");
fflush(stdout);
/*设置默认的搜索路径*/
getcwd(PWD, sizeof(PWD));
//保存 shell 路径
strcpy(SHELL, PWD);
return;
}
//显示提示符
//提示符格式:[P]用户名@主机名:路径$
void type_prompt(){
struct passwd* username;
char hostname[255];
char pwd[255];
username = getpwuid(getuid());
//获得当前用户信息,结构中的 pw_name 为用户名
gethostname(hostname, sizeof(hostname));//获得主机名
getcwd(pwd, sizeof(pwd));
//获得当前工作路径
strcpy(PWD, pwd);
printf("\033[5m[P]\033[0;1;32m%s@%s:\033[34m", username->pw_name, hostname); //显示[P]用户名@主机名:闪烁的,用于区分系统 shell
if (strncmp(pwd, username->pw_dir, strlen(username->pw_dir)) == 0) //如果当前路径在用户路径下,用~代替
printf("~%s\033[0m", pwd + strlen(username->pw_dir));
//打印路径
else
printf("%s\033[0m", pwd);
//否则显示完整路径
printf("$ ");
//显示提示符$
memset(cmd_line, 0, sizeof(cmd_line));
//清空命令内容
memset(pipe_cmd_line, 0, sizeof(cmd_line));
//清空管道命令内容
memset(outputfile, 0, sizeof(outputfile));
//清空输出文件内容
memset(buffer, 0, sizeof(buffer));
//清空输出缓冲区内容
memset(cmd_args, 0, sizeof(cmd_args));
//清空内容
redirect = 0;
bg = 0;
mypipe = 0;
return;
}
//从键盘缓冲区读取命令
//成功返回 0,字符超出限制返回-1
int read_command(){
fgets(cmd_line, sizeof(cmd_line), stdin);
if (cmd_line[strlen(cmd_line) - 1] == '\n') {
cmd_line[strlen(cmd_line) - 1] = '\0';
//fgets 不填'\0',将'\n'替换'\0'
}
else
return -1;
//如果溢出返回错误
return 0;
}
//获取命令的各个参数保存到 cmd_args 中
void get_cmd_args(){
char* ptr = cmd_line;
int num = 0;
while(*ptr != '\0') {
while (*ptr == ' ' || *ptr == '\t')
//清除开头的空格
ptr++;
char options[MAX_LINE];
char* optr = options;
while ( *ptr != ' ' && *ptr != '\t' && *ptr != '&' && *ptr != '\0')
*optr++ = *ptr++;
//字母、数字内容被保存
*optr = '\0';
if(*options){
cmd_args[num] = (char*)malloc(sizeof(char)*(strlen(options)+1));
strcpy(*(cmd_args + num),options);
num++;
}
if(*ptr == '&') ptr++;
}
}
//检查>> > | &
void check_cmd_line(){
char* pos;
if (pos=strstr(cmd_line, " < "));
//判定是否有输入重定向符
if (pos=strstr(cmd_line, " >> ")){
//判定是否有输出重定向符
*pos = '\0';
pos=pos+4;
while (*pos == ' ' || *pos == '\t')
//去除空格
pos++;
strcpy(outputfile,pos);
//把 >> 后的输出路径保存
//printf("%s\n",outputfile);
redirect = 2;
//2 为追加
}
else if (pos=strstr(cmd_line, " > ")) {
//判定是否有输出重定向符
*pos = '\0';
pos=pos+3;
while (*pos == ' ' || *pos == '\t')
//去除空格
pos++;
strcpy(outputfile,pos);
//把 > 后的输出路径保存
//printf("%s\n",outputfile);
redirect = 1;
//1 为覆盖
}
if (pos=strstr(cmd_line, "|")){
//判定是否有管道符
*pos = '\0';
//删除|
pos=pos+1;
while (*pos == ' ' || *pos == '\t')
//去除空格
pos++;
strcpy(pipe_cmd_line, pos);
//把 | 后的下条命令保存
//printf("%s \n", pipe_cmd_line);
mypipe = 1;
}
if (pos=strstr(cmd_line, "&")){
//判定是否有并行运算符
bg = 1;
*pos = '\0';
//删除&
}
get_cmd_args();
//提取参数
}
//输出缓存区输出重定向,将输出缓存区的内容输出到文件
void redirect_output(){
int fd;
int pid;
if (outputfile == NULL) {
//如果没有文件
printf("Phantom shell: 请输入重定向文件\n");
return;
}
if(outputfile[0] == '~'){
//识别~,自动替换为主目录
struct passwd* homepath;
//获取主目录
homepath = getpwuid(getuid());
char pwd[MAX_LINE];
strcpy(pwd,outputfile);
char *ptr = strchr(pwd, '/');
strcpy(outputfile,homepath->pw_dir);
strcat(outputfile,ptr);
}
if (redirect == 1) {
//覆盖原内容
fd = open(outputfile, O_CREAT | O_TRUNC | O_WRONLY, 0644);
//打开文件,若不在,则创建
if (fd < 0) {
printf("Phantom shell: 文件 %s 打开失败!\n",outputfile);
}
pid = fork();
if (pid > 0) {
//父进程等待子进程输出
waitpid(pid, NULL, 0);//
}
else if (pid == 0) {
//子进程把 stdout 重定向到 fd 文件,输出缓冲区内内容
dup2(fd, 1);
printf("%s",buffer);
exit(0);
}
else
return;
close(fd);
}
else if (redirect == 2){
//追加内容
fd = open(outputfile, O_CREAT | O_APPEND | O_WRONLY, 0644); //打开文件,若不在,则创建
if (fd < 0) {
printf("Phantom shell: 文件 %s 打开失败!\n",outputfile);
}
pid = fork();
if (pid>0) {
//父进程等待子进程
waitpid(pid, NULL, 0);//
}
else if (pid == 0) {//子进程把 stdout 重定向到 fd 文件,输出缓冲区内内容
dup2(fd, 1);
printf("%s",buffer);
exit(0);
}
else
return;
close(fd);
}
}
//外部命令输出重定向
void redirect_extern(){
int fd;
int pid;
if (outputfile == NULL) {
//如果没有文件
printf("Phantom shell: 请输入重定向文件\n");
return;
}
if(outputfile[0] == '~'){
//识别~,自动替换为主目录
struct passwd* homepath;
//获取主目录
homepath = getpwuid(getuid());
char pwd[MAX_LINE];
strcpy(pwd,outputfile);
char *ptr = strchr(pwd, '/');
strcpy(outputfile,homepath->pw_dir);
strcat(outputfile,ptr);
}
if (redirect == 1) {
//覆盖原内容
fd = open(outputfile, O_CREAT | O_TRUNC | O_WRONLY, 0644);
//打开文件,若不在,则创建
if (fd < 0) {
printf("Phantom shell: 文件 %s 打开失败!\n",outputfile);
}
else
dup2(fd, 1);
//输出重定向到文件
close(fd);
}
else if (redirect == 2){
//追加内容
fd = open(outputfile, O_CREAT | O_APPEND | O_WRONLY, 0644); //打开文件,若不在,则创建
if (fd < 0) {
printf("Phantom shell: 文件 %s 打开失败!\n",outputfile);
}
else
dup2(fd, 1);
//输出重定向到文件
close(fd);
}
}
int main(void){
int should_run = 1; //决定是否继续运行
int pid;
signal(SIGINT, SIG_IGN); //不接收 Ctrl+C
signal(SIGTSTP, SIG_IGN); //不接收 Ctrl+Z
init();
//初始化
while (should_run) {
type_prompt(); //显示提示符
if (read_command() == -1){
printf("命令过长!最大长度为:%d\n", MAX_LINE);
continue;
}
check_cmd_line();
//int i = 0;
//while(cmd_args[i])
//printf(" %s",cmd_args[i++]);
//printf("\n");
//printf(" %s\n",outputfile);
if (is_internal_cmd()) { //处理内部命令
if(redirect)
//是否需要重定向
redirect_output();
else
printf("%s",buffer);
continue;
}
if (mypipe)
//是否有管道
pipe(fd);
pid = fork();
if (pid == 0) {
//子进程
signal(SIGINT, SIG_DFL); //接收 Ctrl+C
signal(SIGTSTP, SIG_DFL); //接收 Ctrl+Z
signal(SIGCONT, SIG_DFL); //接收 bg、fg 发送的继续信号
if (mypipe) {
//如果有管道,关闭读端口,输出定位到写端口
close(fd[0]); //将本命令输出到管道
dup2(fd[1], 1);
}
if(redirect)
redirect_extern();
exec_external_cmd(); //处理外部命令
break;
}
else if (pid>0) {
//父进程
if (bg == 1) {
//判断是否并行执行
printf("该后台任务的 PID 为: %d\n", pid); //输出该进程的标识号
signal(SIGCHLD, SIG_IGN);
//把该子进程交给 init 系统进程管理,避免产生僵尸进程
}
else {
waitpid(pid, &state, 0); //父进程阻塞,等待子进程结束 WUNTRACED
}
if (mypipe)
using_pipe();
//处理管道
}
else continue;
}
return 0;
}
//管道处理函数,将下一条命令的输入设置为管道
void using_pipe(){
int pid;
pid = fork();
if (pid == 0) {
//子进程,关闭写端口,从 fd[0]管道读端口获得内容
close(fd[1]);
dup2(fd[0], 0);
memset(cmd_args, 0, sizeof(cmd_args));
//清空原参数
strcpy(cmd_line, pipe_cmd_line);
//读入下条命名
get_cmd_args();
//提取参数
exec_external_cmd();
//外部运行该命令
}
else {
//父进程阻塞,等待子进程结束
close(fd[1]);
waitpid(pid, &state, 0);
}
}
//执行外部命令,使用 execvp 调用
void exec_external_cmd(){
if ( execvp(cmd_args[0], cmd_args) < 0)
//如果返回值小于 0,说明没有找到该命令
printf("Phantom Shell: 命令 %s 没有找到\n",cmd_args[0]);
}
//执行内部命令,没有查询到返回 0,跳过输出
int is_internal_cmd(){
if (cmd_args[0] == NULL || strcmp(cmd_args[0], "|") == 0 || strcmp(cmd_args[0], "&") == 0 || cmd_args[0][0]
== '\0') //排除空命令
return 0;
else if (strcmp(cmd_args[0], "stop") == 0) {
//stop 命令用于暂停指定进程
int pid;
if (cmd_args[1] != NULL) {
pid = atoi(cmd_args[1]);
//将 PID 从字符串转换为整数
}
else {
//没有 PID 时
printf("Phantom Shell: stop: 缺少参数\n");
return 1;
}
if (kill(pid,SIGSTOP) < 0) {
//暂停该进程
printf("Phantom Shell: bg: 该进程不存在\n");
}
return 1;
}
else if (strcmp(cmd_args[0], "bg") == 0) {
int pid;
if (cmd_args[1] != NULL) {
pid = atoi(cmd_args[1]);
//bg 命名使挂起的进程继续在后台运行
//将 PID 从字符串转换为整数
}
else {
//没有 PID 时
printf("Phantom Shell: bg: 缺少参数\n"); //打印提示信息
return 1;
}
if (kill(pid, SIGCONT) < 0) {
//使进程继续
printf("Phantom Shell: bg: 该进程不存在\n");
}
else
waitpid(pid, &state, WUNTRACED);
return 1;
}
else if (strcmp(cmd_args[0], "fg") == 0) {
//fg 命令将后台运行的进程切换到前台运行
int pid;
if (cmd_args[1] != NULL) {
pid = atoi(cmd_args[1]);
//将 PID 从字符串转换为整数
}
else {
//没有 PID 时
printf("Phantom Shell: fg: 缺少参数\n");
return 1;
}
if (tcsetpgrp(1, getpgid(pid)) == 0) {
//把该进程切换到前台
kill(pid, SIGCONT);
//使进程继续
waitpid(pid, NULL, WUNTRACED);
//等待该进程结束或暂停
}
else
printf("Phantom Shell: fg: 该进程不存在\n");
//进程不存在情况
return 1;
}
else if (strcmp(cmd_args[0], "cd") == 0) {
shell_cd();
return 1;
}
else if (strcmp(cmd_args[0], "clr") == 0) {
printf("\033[2J\033[0;0H");
return 1;
} //cd 命令用于切换目录
else if (strcmp(cmd_args[0], "dir") == 0) {
shell_dir();
return 1;
} //dir 命令用于显示指定目录下的内容
//clr 命令用于清屏
else if (strcmp(cmd_args[0], "exit") == 0 || strcmp(cmd_args[0], "quit") == 0) { //exit、quit 命令用于退出
exit(0);
}
else if (strcmp(cmd_args[0], "echo") == 0) {
//echo 命令用于显示指定内容
for (int i = 1; cmd_args[i]; i++) {
if(strcmp(cmd_args[i], ">>") == 0 || strcmp(cmd_args[i],">") == 0 )
//读到重定向符时结束
break;
strcat(buffer, cmd_args[i]); strcat(buffer," ");
//将全部参数写入输出缓冲区,用一个空格隔开
}
strcat(buffer,"\n");
return 1;
}
else if (strcmp(cmd_args[0], "environ") == 0) {
//environ 命令用于显示环境变量
for (int i = 0; environ[i] != NULL; i++) {
strcat(buffer,environ[i]); strcat(buffer,"\n");
//将环境变量写入输出缓存区
}
return 1;
}
else if (strcmp(cmd_args[0], "exec") == 0) {
//exec 用于调用并执行命令
if (cmd_args[1] == NULL){
//检查参数
printf("Phantom Shell: exec: 缺少参数\n");
return 1;
}
if (execvp(cmd_args[1], cmd_args + 1) < 0)
//如果返回值小于 0,说明没有找到指定命令
printf("Phantom Shell: exec: 命令 %s 没有找到\n",cmd_args[1]);
return 1;
}
else if (strcmp(cmd_args[0], "help") == 0) {
shell_help();
return 1;
} //help 命令显示帮助
else if (strcmp(cmd_args[0], "jobs") == 0) {
shell_jobs();
return 1;
}
else if (strcmp(cmd_args[0], "pwd") == 0) {
strcat(buffer,PWD); strcat(buffer,"\n");
//printf("%s\n", PWD);
return 1;
} //jobs 命令显示当前全部进程
else if (strcmp(cmd_args[0], "test") == 0) {
shell_test();
return 1;
}
else if (strcmp(cmd_args[0], "time") == 0) {
time_t timep;
time (&timep);
strcat(buffer,ctime(&timep));
return 1;
}
else if (strcmp(cmd_args[0], "umask") == 0) {
shell_umask();
return 1;
} //test 命令用来检查文件类型
else if (strcmp(cmd_args[0], "myshell") == 0) {
if (cmd_args[1] == NULL){
//缺少参数
printf("Phantom Shell: myshell: 缺少参数\n");
return 1;
}
else {
myshell();
return 1;
}
}
//pwd 用于打印当前工作目录
//time 命令用于显示当前时间
//umask 命令用于查看或者设置 umask 掩码
//myshell 命令执行文本里的命令
else if (strcmp(cmd_args[0], "shift") == 0) {
//shift 命令左移参数
if (cmd_args[1] == NULL){
//缺少参数
printf("Phantom Shell: shift: 缺少参数\n");
return 1;
}
else {
int num = atoi(cmd_args[1]);
int i = 2;
while(cmd_args[i]) {cmd_args[i] = cmd_args[i+num]; i++;}
i = 2;
while(cmd_args[i]) printf("%s ",cmd_args[i++]);
printf("\n");
}
return 1;
}
else if (strcmp(cmd_args[0], "set") == 0) {
//set 命令用于查看或设置指定环境变量
char * ptr;
if (cmd_args[1] == NULL){
//缺少参数
printf("Phantom Shell: set: 缺少参数\n");
return 1;
}
if (cmd_args[2] == NULL){
//显示环境变量
ptr = getenv(cmd_args[1]);
if(ptr){
strcpy(buffer,cmd_args[1]);
strcat(buffer," = ");
strcat(buffer,ptr);
strcat(buffer,"\n");
}
else
printf("Phantom Shell: set: %s 变量不存在\n" , cmd_args[1]);
return 1;
}
else
//修改环境变量
setenv(cmd_args[1],cmd_args[2],1);
return 1;
}
else if (strcmp(cmd_args[0], "unset") == 0) {
//unset 命令用于删除环境变量
if (cmd_args[1] == NULL){
//缺少参数
printf("Phantom Shell: unset: 缺少参数\n");
return 1;
}
if(unsetenv(cmd_args[1]))
//删除参数
printf("Phantom Shell: unset: %s 变量不存在\n" , cmd_args[1]);
return 1;
}
else return 0;
}
//cd
void shell_cd(){
struct passwd* homepath;
//获取主目录
homepath = getpwuid(getuid());
if( cmd_args[1] == NULL || strcmp(cmd_args[1], "~") == 0 ) { //缺少参数
if( chdir(homepath->pw_dir) == -1 ) {
printf("Phantom Shell: cd: %s: 没有那个文件或目录\n",cmd_args[1]);
return;
}
}
else if ( strcmp(cmd_args[1], "..") == 0 ){
//..返回上级目录
char pwd[MAX_LINE];
getcwd(pwd, sizeof(pwd));
//删除当前目录中最后一个/后的内容获得父目录
char *ptr = strrchr(pwd, '/'); *ptr = '\0';
if( chdir(pwd) == -1 ) {
printf("Phantom Shell: cd: %s: 没有那个文件或目录\n",cmd_args[1]);
return;
}
}
else if ( strcmp(cmd_args[1], ".") == 0 );
//.跳过
else{
//其他路径
if(cmd_args[1][0] == '~'){
//识别~,自动替换为主目录
char pwd[MAX_LINE];
strcpy(pwd,cmd_args[1]);
char *ptr = strchr(pwd, '/');
strcpy(cmd_args[1],homepath->pw_dir);
strcat(cmd_args[1],ptr);
}
if(chdir(cmd_args[1])==-1){
printf("Phantom Shell: cd: %s: 没有那个文件或目录\n",cmd_args[1]);
return;
}
}
getcwd(PWD, sizeof(PWD)); //更新 PWD
}
//umask
void shell_umask(){
char otostr[6]={0};
if( cmd_args[1] == NULL) {
sprintf(otostr,"%04o\n",my_umask);
strcpy(buffer,otostr);
return;
}
else{
//修改
int temp;
sscanf( cmd_args[1], "%o", &temp);
if(temp < 0 || temp > 02777)
printf("Phantom Shell: umask:%o: 八进制数越界\n",temp);
else
my_umask = temp;
}
//显示
//按八进制形式写入输出缓存区
//按照八进制读入参数
}
//dir
void shell_dir(){
DIR *dir;
//保存目录信息
struct dirent *dp;
if( cmd_args[1] == NULL) {
//如果没有参数
cmd_args[1]=PWD;
}
if(cmd_args[1][0] == '~'){
//识别~,自动替换为主目录
struct passwd* homepath;
//获取主目录
homepath = getpwuid(getuid());
char pwd[MAX_LINE];
strcpy(pwd,cmd_args[1]);
char *ptr = strchr(pwd, '/');
strcpy(cmd_args[1],homepath->pw_dir);
strcat(cmd_args[1],ptr);
}
//printf("%s\n",cmd_args[1]);
dir = opendir(cmd_args[1]);
//获得文件的指针
while ((dp = readdir(dir)) != NULL) {//列出得到的信息
strcat(buffer, dp->d_name);strcat(buffer, "\n");
//printf("%s\n",dp->d_name);
}
return;
}
void shell_help(){
if(cmd_args[1] == NULL){ //缺省参数时
char* menu = "Phantom Shell,2018.8,作者:徐金焱\n 这些 shell 命令是内部定义的。输入 `help 名称' 以得到有关函数`名称'的更多信息。\nbg [进程 PID]\t\t 使挂起的进程继续在后台运行\ncd [目录]\t\t 改变 shell 工作目录\nclr\t\t 清除屏幕\ndir [目录]\t\t 显示指定目录下的内容\necho [参数 ...] \t\t 显示指定内容\nexec[命令 [参数 ...]]\t\t 调用并执行命令\nexit\t\t 退出 shell\nenviron\t\t 显示环境变量\nfg [进程 PID]\t\t 将后台运行的进程切换到前台运行\nhelp\t\t 显示帮助\njobs\t\t 显示当前全部进程\nmyshell [文件]\t\t 执行文本里的命令\npwd\t\t 打印当前工作目录\nquit\t\t 退出 shell\nset [变量名] [变量值]\t\t 查看或设置指定环境变量\nshift [位移数] [参数 ...]\t 左移参数\nstop [进程 PID]\t\t 暂停指定进程\ntest [选项][文件] \t\t 检查文件类型\ntime\t\t 显示当前时间\numask [掩码]\t\t 查看或者设置 umask 掩码\nunset [变量名]\t\t 删除环境变量\n";
strcat(buffer, menu);
}
else{
if (strcmp(cmd_args[1], "bg") == 0) {
char* info = "bg: bg [进程 PID]\n\t 使挂起的进程继续在后台运行,与带 `&' 启动的一样。\n\t 通过 PID 来选定进程。\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "cd") == 0) {
char* info = "cd: cd [目录]\n\t 改变 shell 工作目录。\n\t 支持~简化输入,以及..返回上级目录。\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "clr") == 0) {
char* info = "clr:clr\n\t 清除屏幕。\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "dir") == 0) {
char* info = "dir: dir [目录]\n\t 显示指定目录下的内容。\n\t 支持~简化输入目录。\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "echo") == 0) {
char* info = "echo: echo [参数 ...]\n\t 显示指定内容。\n\t 内容即为参数,将自动合并多余的空格。\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "exec") == 0) {
char* info = "exec: exec [命令 [参数 ...]]\n\t 使用指定命令替换 shell。\n\t 执行 COMMAND 命令,以指定的程序替换这个 shell。\n\tARGUMENTS 参数成为 COMMAND 命令的参数。\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "exit") == 0) {
char* info = "exit: exit\n\t 退出 shell。\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "environ") == 0) {
char* info = "environ: environ\n\t 显示环境变量。\n\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "fg") == 0) {
char* info = "fg: fg [进程 PID] \n\t 将后台运行的进程切换到前台运行。\n\t 通过 PID 来选定进程。\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "help") == 0) {
char* info = "help: help [命令名]\n\t 显示帮助。\n\t 当没有参数时,显示全部命令的简略介绍。\n\t 有变量名参数时,显示该命令的具体介绍。\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "jobs") == 0) {
char* info = "jobs: jobs\n\t 显示当前全部进程。 \n\t 显示内容为只与当前 shell 有关的进程, \n\t 且只有 PID、 COMMAND两项。\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "myshell") == 0) {
char* info = "myshell: myshell [文件]\n\t 执行文本里的命令。\n\t 文件中每条命令通过回车分割。\n\t 支持~简化输入目录。\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "pwd") == 0) {
char* info = "pwd: pwd\n\t 打印当前工作目录。\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "quit") == 0) {
char* info = "quit: quit\n\t 退出 shell。\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "set") == 0) {
char* info = "set: set [变量名] [变量值]\n\t 查看或设置指定环境变量。\n\t 当变量值为空时,返回当前变量的值。\n\t 当变量值非空时,修改其值。\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "shift") == 0) {
char* info = "shift: shift [位移数] [参数 ...]\n\t 左移参数。\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "stop") == 0) {
char* info = "stop: stop [进程 PID]\n\t 暂停指定进程。\n\t 通过 PID 来选定进程。\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "test") == 0) {
char* info = "test: test [选项] [文件]\n\t 检查文件类型。\n\t 能够区分的类型有:普通文件、目录文件、字符设备文件、\n\t 块设备、管道文件符号连接文件、socket 文件。\n\t 支持~简化输入目录。\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "time") == 0) {
char* info = "time: time\n\t 显示当前时间。\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "umask") == 0) {
char* info = "umask: umask [掩码] \n\t 查看或者设置 umask 掩码。\n\t 当掩码为空时,显示当前掩码。\n\t 掩码非空时,修改其值。\n";
strcat(buffer, info);
}
else if (strcmp(cmd_args[1], "unset") == 0) {
char* info = "unset: unset [变量名]\n\t 删除环境变量。\n";
strcat(buffer, info);
}
}
}
//jobs
void shell_jobs(){
int pid;
pid = fork();
if (pid == 0) {
//子进程调用 ps -o pid,comm 显示当前进程
execlp("ps", "ps", "-o", "pid,comm", NULL);
}
else if (pid > 0) {
//父进程阻塞,等待子进程结束
waitpid( pid, NULL, 0 );
}
else {
printf("后台进程显示失败!\n");
}
return;
}
//test
void shell_test(){
if ( cmd_args[1] == NULL) {
//缺少参数
printf("Phantom Shell: test: 缺少参数\n");
}
else {
struct stat buf;
if(cmd_args[1][0] == '~'){
//识别~,自动替换为主目录
struct passwd* homepath;
//获取主目录
homepath = getpwuid(getuid());
char pwd[MAX_LINE];
strcpy(pwd,cmd_args[1]);
char *ptr = strchr(pwd, '/');
strcpy(cmd_args[1],homepath->pw_dir);
strcat(cmd_args[1],ptr);
}
if (stat(cmd_args[1], &buf) < 0) { //文件不存在时,输出错误信息
printf("Phantom Shell: test: 文件 %s 不存在\n", cmd_args[1]);
}
else {
//判断类型
char* ptr;
if (S_ISREG(buf.st_mode))
ptr = "普通文件";
else if (S_ISDIR(buf.st_mode))
ptr = "目录文件";
else if (S_ISCHR(buf.st_mode))
ptr = "字符设备文件";
else if (S_ISBLK(buf.st_mode))
ptr = "块设备";
else if (S_ISFIFO(buf.st_mode))
ptr = "管道文件";
else if (S_ISLNK(buf.st_mode))
ptr = "符号连接文件";
else if (S_ISSOCK(buf.st_mode))
ptr = "socket 文件";
else
ptr = "未知文件";
strcat(buffer, ptr); strcat(buffer,"\n"); //将类型输出到显示缓存区
}
}
}
//myshell
void myshell(){
FILE *inputfile;
if(cmd_args[1][0] == '~'){
//识别~,自动替换为主目录
struct passwd* homepath;
//获取主目录
homepath = getpwuid(getuid());
char pwd[MAX_LINE];
strcpy(pwd,cmd_args[1]);
char *ptr = strchr(pwd, '/');
strcpy(cmd_args[1],homepath->pw_dir);
strcat(cmd_args[1],ptr);
}
inputfile = fopen(cmd_args[1], "r");
//打开输入文件
if (inputfile == NULL) {
printf("Phantom Shell: myshell: 文件不存在 %s", cmd_args[1]);
}
else {
int pos_after_pipe, bg, num = 0;
pid_t pid;
char* cptr;
while (!feof(inputfile)) {
//文件没有结束
//初始化参数
memset(cmd_line, 0, sizeof(cmd_line));
memset(pipe_cmd_line, 0, sizeof(cmd_line));
memset(outputfile, 0, sizeof(outputfile));
memset(buffer, 0, sizeof(buffer));
memset(cmd_args, 0, sizeof(cmd_args));
redirect = 0;
bg = 0;
mypipe = 0;
fgets(cmd_line, MAX_LINE, inputfile); //从文件中读取一行
cmd_line[strlen(cmd_line) - 1] = '\0';
check_cmd_line();
//下面内容同 main
if (is_internal_cmd()) {
if(redirect)
redirect_output();
else
printf("%s",buffer);
continue;
}
if (mypipe)
pipe(fd);
pid = fork();
if (pid == 0) {
if (mypipe) {
close(fd[0]);
dup2(fd[1], 1);
}
exec_external_cmd();
break;
}
else if (pid>0) {
if (bg == 1) {
printf("该后台任务的 PID 为: %d\n", pid);
signal(SIGCHLD, SIG_IGN);
}
else {
waitpid(pid, &state, WUNTRACED);
}
if (mypipe) using_pipe();
}
else continue;
}
}
fclose(inputfile);
}


