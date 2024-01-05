#include <stdio.h>
int main(){
volatile long sum = 0;
long i;for(i = 0; i < 2830000000; i++){
sum++;}system("touch 1_48.txt");return 0;}
