#!/usr/bin/Rscript


t <- read.table(file("stdin"),header=F)

#argv <- commandArgs()
#cutoff <- argv[6]
#print(cutoff)

cutoff <- 1

pos <- t[t$V2>=cutoff,1]
neg <- t[t$V2<cutoff,1]

# print(wilcox.test(pos,neg))

alle <- sort(unique(c(pos,neg)))
alle <- c(alle,max(alle)+1)

falsepositiverate <- c()
truepositiverate <- c()

for( i in alle ){
  falsepositiverate <- c( length(neg[neg>=i])/length(neg), falsepositiverate )
  truepositiverate <- c( length(pos[pos>=i])/length(pos), truepositiverate )
}

# numerical integral of a function 
trap.rule <- function(x,y) sum(diff(x)*(y[-1]+y[-length(y)]))/2 

cat(trap.rule(falsepositiverate,truepositiverate),"\n")

