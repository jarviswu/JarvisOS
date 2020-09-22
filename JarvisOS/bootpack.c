/* bootpack�̃��C�� */

#include "bootpack.h"
#include <stdio.h>
#define MEMMAN_FREES 4090  /*4090个FREEINFO，大小约为4090*8 = 32KB*/
#define MEMMAN_ADDR 0x003c0000
// 可用信息
struct FREEINFO { 
	unsigned int addr, size;
};

// 内存管理
struct MEMMAN {
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};

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


unsigned int memtest(unsigned int start, unsigned int end);

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40], mcursor[256], keybuf[32], mousebuf[128];
	int mx, my, i;
	struct MOUSE_DEC mdec;
	unsigned int memtotal;
	struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;

	init_gdtidt();
	init_pic();
	io_sti(); /* IDT/PIC�̏��������I������̂�CPU�̊��荞�݋֎~������ */
	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 128, mousebuf);
	io_out8(PIC0_IMR, 0xf9); /* PIC1�ƃL�[�{�[�h������(11111001) */
	io_out8(PIC1_IMR, 0xef); /* �}�E�X������(11101111) */

	init_keyboard();
	enable_mouse(&mdec);

	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
	mx = (binfo->scrnx - 16) / 2; /* ��ʒ����ɂȂ�悤�ɍ��W�v�Z */
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf(s, "(%3d, %3d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000);
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	sprintf(s, "memory %dMB   free : %dKB",
            memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

	for (;;) {
		io_cli();
		if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
			io_stihlt();
		} else {
			if (fifo8_status(&keyfifo) != 0) {
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(s, "%02X", i);
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484,  0, 16, 15, 31);
				putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
			} else if (fifo8_status(&mousefifo) != 0) {
				i = fifo8_get(&mousefifo);
				io_sti();
				if (mouse_decode(&mdec, i) != 0) {
					/* �f�[�^��3�o�C�g�������̂ŕ\�� */
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01) != 0) {
						s[1] = 'L';
					}
					if ((mdec.btn & 0x02) != 0) {
						s[3] = 'R';
					}
					if ((mdec.btn & 0x04) != 0) {
						s[2] = 'C';
					}
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
					putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
					/* �}�E�X�J�[�\���̈ړ� */
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15); /* �}�E�X���� */
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					if (mx > binfo->scrnx - 16) {
						mx = binfo->scrnx - 16;
					}
					if (my > binfo->scrny - 16) {
						my = binfo->scrny - 16;
					}
					sprintf(s, "(%3d, %3d)", mx, my);
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15); /* ���W���� */
					putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s); /* ���W���� */
					putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16); /* �}�E�X�`�� */
				}
			}
		}
	}
}

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

unsigned int memtest(unsigned int start, unsigned int end)
{
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	/* 386���A486�ȍ~�Ȃ̂��̊m�F */
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if ((eflg & EFLAGS_AC_BIT) != 0) { /* 386�ł�AC=1�ɂ��Ă�������0�ɖ߂��Ă��܂� */
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT; /* AC-bit = 0 */
	io_store_eflags(eflg);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE; /* �L���b�V���֎~ */
		store_cr0(cr0);
	}

	i = memtest_sub(start, end);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE; /* �L���b�V������ */
		store_cr0(cr0);
	}

	return i;
}
