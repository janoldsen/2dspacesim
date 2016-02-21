#ifndef ATOMIC_H

typedef int atomic;

void atomicAdd(atomic* pAtomic, int value);
void atomicSub(atomic* pAtomic, int value);
void atomicIncr(atomic* pAtomic);
void atomicDecr(atomic* pAtomic);

int atomicCmpEx(atomic* pAtomic, int ex, int cmp);

#endif
