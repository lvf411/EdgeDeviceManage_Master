#include <stdio.h>
int main(){
volatile long sum = 0;
long i;for(i = 0; i < 1060000000; i++){
sum++;}system("touch 1_44.txt");return 0;}
