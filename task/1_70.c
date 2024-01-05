#include <stdio.h>
int main(){
volatile long sum = 0;
long i;for(i = 0; i < 2670000000; i++){
sum++;}system("touch 1_70.txt");return 0;}
