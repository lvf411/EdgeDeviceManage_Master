#include <stdio.h>
int main(){
volatile long sum = 0;
long i;for(i = 0; i < 2330000000; i++){
sum++;}system("touch 1_31.txt");return 0;}
