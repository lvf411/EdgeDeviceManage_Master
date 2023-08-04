#include "kbhit.hpp"

//非阻塞获取键盘状态，当没有按键按下时返回0，有按键按下时返回按下字符的ASCII码
int kbhit()
{
    struct termios oldt, newt;
    char ch;
    int oldf;
    //printf("m\n");
    //sleep(1);
    //获取键盘输入的相关参数存储在oldt内
    tcgetattr(STDIN_FILENO, &oldt);
    //将相关参数赋值给新的io结构体实例
    newt = oldt;
    //不使用标准输入模式，不显示获取字符
    newt.c_lflag &= ~(ICANON /*| ECHO*/);
    //不等数据传输完毕就立即改变键盘输入的属性
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    //获取键盘输入的状态标志
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    //为键盘输入添加非阻塞的状态标志
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    //立即输入，复原原先的键盘输入参数
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    //复原原先的状态标志
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    //非阻塞捕获到输入信号且不为漏下的换行符，返回1
    if(ch != EOF && ch != '\n')
    {
    	//printf("%c", ch);
        //将捕获到的数据重新放回输入缓冲区，下次读取时第一个读到
        ungetc(ch, stdin);
        return ch;
    }
    //非阻塞未捕获到输入信号，返回0
    return 0;
}
