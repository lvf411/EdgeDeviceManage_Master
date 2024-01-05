#include <stdio.h>
int main(){
volatile long sum = 0;
long i;for(i = 0; i < 3940000000; i++){
sum++;}system("echo a > 1_2.txt");return 0;}
