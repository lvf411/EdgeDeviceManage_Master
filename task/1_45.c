#include <stdio.h>
int main(){
volatile long sum = 0;
long i;for(i = 0; i < 510000000; i++){
sum++;}system("touch 1_45.txt");return 0;}
