#include <stdio.h>
int main(){
volatile long sum = 0;
long i;for(i = 0; i < 3130000000; i++){
sum++;}system("touch 1_63.txt");return 0;}
