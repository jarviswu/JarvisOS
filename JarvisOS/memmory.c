#include "bootpack.h"

void memman_init(struct MEMMAN *man) {
	// 可用信息数目
	man->frees = 0;
	// frees的最大值
	man->maxfrees = 0;
	// 释放失败的内存大小总和
	man->lostsize = 0;
	// 释放失败的次数
	man->losts = 0;
	return;
}

// 剩余内存大小
unsigned int memman_total(struct MEMMAN *man) {
	unsigned int i, t = 0;
	for (i = 0; i< man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}

// 分配指定大小的内存
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size) {
	unsigned int address = 0;
	unsigned int i = 0;
	for (i = 0; i < man->frees; ++i) {
		// 找到大小合适的内存块
		if (man->free[i].size > size) {
			address = man->free[i].addr;
			man->free[i].size -= size;
			man->free[i].addr += size;
			if (man->free[i].size == 0) {
				// 该块大小size为0，可用大小减一，并且将数组后面的内存挪动到前面
				--man->frees;
				unsigned int j = i;
				for (j = i; j < man->frees && j + 1 < man->frees; ++j) {
					man->free[j] = man->free[j+1];
				}
			}

			return address;
		}
	}
	return address;
}

int memman_free(struct MEMMAN * man, unsigned int addr, unsigned int size) {
    int i, j;
    // 为便于归纳内存，将free[]按照addr的顺序排列
    // 所以，先决定应该放在哪里
    for (i = 0; i < man->frees; i++) {
        if (man->free[i].addr > addr) {
            break;
        }
    }
    // free[i - 1].addr < addr < free[i].addr
    if (i > 0) {
        // 前面有可用内存
        if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
            // 可以与前面的可用内存归纳到一起
            man->free[i - 1].size += size;
            if (i < man->frees) {
                // 后面也有
                if (addr + size == man->free[i].addr) {
                    // 也可以与后面的可用内存归纳到一起
                    man->free[i - 1].size += man->free[i].size;
                    // man->free[i]删除 
                    // free[i]变成0后归纳到前面去
                    man->frees--;
                    for (; i < man->frees; i++) {
                        man->free[i] = man->free[i + 1]; // 结构体赋值
                    }
                }
            }
            return 0; // 成功完成
		}
    }
    // 不能与前面的可用空间归纳到一起
    if (i < man->frees) {
        // 后面还有
        if (addr + size == man->free[i].addr) {
            // 可以与后面的内容归纳到一起
            man->free[i].addr = addr;
            man->free[i].size += size;
            return 0; 
        }
    }
    // 既不能与前面归纳到一起，也不能与后面归纳到一起
    if (man->frees < MEMMAN_FREES) {
        // free[i]之后的，向后移动，腾出一点可用空间
        for (j = man->frees; j > i; j--) {
            man->free[j] = man->free[j - 1];
        }
        man->frees++;
        if (man->maxfrees < man->frees) {
            man->maxfrees = man->frees; 
        }
        man->free[i].addr = addr;
        man->free[i].size = size;
        return 0; 
    }
    // 不能往后移动
    man->losts++;
    man->lostsize += size;
	return -1;
}

// 一次分配4KB
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
    unsigned int a;
    // 以4KB（0x1000）为单位向上取整
    size = (size + 0xfff) & 0xfffff000;
    a = memman_alloc(man, size);
    return a;
}

// 一次释放4KB
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
    int i;
    size = (size + 0xfff) & 0xfffff000;
    i = memman_free(man, addr, size);
    return i;
}
