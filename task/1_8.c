#include <stdio.h>
int main(){
volatile long sum = 0;
long i;for(i = 0; i < 950000000; i++){
sum++;}system("echo a > 1_8.txt");return 0;}
