#include <stdio.h>
#include <stdint.h>
#pragma warning(disable : 4996)

struct cachePage
{
	uint8_t valid = 0;
	uint8_t address = 0;
	uint8_t dirty = 0;
};

struct MMU
{
	uint8_t leastRecent = 0;
	uint8_t pageCount = 2;
	cachePage pages[2];
};

uint8_t pcIncrement = 0;

uint8_t pc;
uint8_t registers[8];

uint8_t program[256];
MMU memory;
uint8_t cache[64];
uint8_t ram[256];

void core_dump();

int main(int argc, char* argv[])
{
	if (argc == 1)
	{
		printf("No file given. Exiting\n");
		return -1;
	}
	FILE* programFile = fopen(argv[1], "r");
	if (programFile == NULL)
	{
		printf("No file found with that name. Exiting\n");
		return -1;
	}
	fseek(programFile, 0L, SEEK_END);
	int fileSize = ftell(programFile);
	fseek(programFile, 0L, SEEK_SET);
	if (programFile == NULL)
	{
		printf("Error opening file\n");
		return -1;
	}
	uint8_t smallerNumber = 255;
	if (fileSize < 255)
	{
		smallerNumber = (uint8_t)(fileSize - 1);
	}
	for (uint8_t index = 0; index < smallerNumber; index++)
	{
		program[index] = fgetc(programFile);
	}
    pc = 0;
	while (pc <= 253)
	{
		uint8_t opcode = program[pc];
		uint8_t arg1 = program[pc + 1];
		uint8_t arg2 = program[pc + 2];
		uint8_t arg3 = 0;
		if (pc != 253)
		{
			arg3 = program[pc + 3];
		}

		if (opcode != (uint8_t)0x20 && opcode != (uint8_t)0x22)
		{
			if (arg1 >= 8)
			{
				printf("Unknown register in Arg1\n");
				core_dump();
				return -1;
			}
		}
		if (arg2 >= 8)
		{
			printf("Unknown register in Arg2\n");
			core_dump();
			return -1;
		}
		if (opcode >= (uint8_t)0x06 && opcode <= (uint8_t)0x0D ||
			opcode >= (uint8_t)0x10 && opcode <= (uint8_t)0x1D)
		{
			if (arg3 >= 8)
			{
				printf("Unknown register in Arg3\n");
				core_dump();
				return -1;
			}
		}

		switch (opcode)
		{



			// ALU::ARITHMETIC



		case (uint8_t)0x00:
			// pass
			// Passes the contents of register Arg1 to Arg2
			registers[arg2] = registers[arg1];
			pcIncrement = 3;
			break;
		case (uint8_t)0x01:
			// neg
			// Negates the contents of Arg1 and saves to Arg2
			registers[arg2] = ~registers[arg1];
			pcIncrement = 3;
			break;
		case (uint8_t)0x02:
			// inc
			// Increments the contents of register Arg1 and saves to Arg2
			registers[arg2] = registers[arg1] + 1;
			pcIncrement = 3;
			break;
		case (uint8_t)0x03:
			// dec
			// Decrements the contents of register Arg1 and saves to Arg2
			registers[arg2] = registers[arg1] - 1;
			pcIncrement = 3;
			break;
		case (uint8_t)0x06:
			// add
			// Adds Arg1 and Arg2 and saves to Arg3
			registers[arg3] = registers[arg1] + registers[arg2];
			pcIncrement = 4;
			break;
		case (uint8_t)0x07:
			// sub
			// Subtracts Arg2 from Arg1 and saves to Arg3
			registers[arg3] = registers[arg1] - registers[arg2];
			pcIncrement = 4;
			break;



			// ALU::LOGIC



		case (uint8_t)0x08:
			// AND
			// Bitwise ANDs Arg1 and Arg2 and saves to Arg3
			registers[arg3] = registers[arg1] & registers[arg2];
			pcIncrement = 4;
			break;
		case (uint8_t)0x09:
			// NAND
			// Bitwise NANDs Arg1 and Arg2 and saves to Arg3
			registers[arg3] = ~(registers[arg1] & registers[arg2]);
			pcIncrement = 4;
			break;
		case (uint8_t)0x0a:
			// NOR
			// Bitwise NORs Arg1 and Arg2 and saves to Arg3
			registers[arg3] = ~(registers[arg1] | registers[arg2]);
			pcIncrement = 4;
			break;
		case (uint8_t)0x0b:
			// OR
			// Bitwise ORs Arg1 and Arg2 and saves to Arg3
			registers[arg3] = registers[arg1] | registers[arg2];
			pcIncrement = 4;
			break;
		case (uint8_t)0x0c:
			// XOR
			// Bitwise XORs Arg1 and Arg2 and saves to Arg3
			registers[arg3] = registers[arg1] ^ registers[arg2];
			pcIncrement = 4;
			break;
		case (uint8_t)0x0d:
			// XNOR
			// Bitwise XNORs Arg1 and Arg2 and saves to Arg3
			registers[arg3] = ~(registers[arg1] ^ registers[arg2]);
			pcIncrement = 4;
			break;
		case (uint8_t)0x0f:
			// NOT
			// Bitwise NOTs Arg1 and saves to Arg2
			registers[arg2] = ~registers[arg1];
			pcIncrement = 3;
			break;



			// ALU::SHIFTS



		case (uint8_t)0x10:
			// shar
			// Arithmetic shifts Arg1 right by Arg2 and saves to Arg3
			registers[arg3] = registers[arg1] >> registers[arg2];
			if (registers[arg1] >> 7 == 0x01)
			{
				registers[arg3] |= (uint8_t)(0xff << (8 - registers[arg2]));
			}
			else
			{
				registers[arg3] &= (uint8_t)~(0xff << (8 - registers[arg2]));
			}
			pcIncrement = 4;
			break;
		case (uint8_t)0x11:
			// shal
			// Arithmetic shifts Arg1 left by Arg2 and saves to Arg3
			registers[arg3] = registers[arg1] << registers[arg2];
			pcIncrement = 4;
			break;
		case (uint8_t)0x12:
			// shlr
			// Logical shifts Arg1 right by Arg2 and saves to Arg3
			registers[arg3] = registers[arg1] >> registers[arg2];
			if (registers[arg1] >> 7 == 0x01)
			{
				registers[arg3] |= (uint8_t)(0xff << (8 - registers[arg2]));
			}
			pcIncrement = 4;
			break;
		case (uint8_t)0x13:
			// shll
			// Logical shifts Arg1 left by Arg2 and saves to Arg3
			registers[arg3] = registers[arg1] << registers[arg2];
			pcIncrement = 4;
			break;
		case (uint8_t)0x14:
			// shrr
			// Rotate shifts Arg1 right by Arg2 and saves to Arg3
			registers[arg3] = registers[arg1] >> registers[arg2];
			registers[arg3] |= registers[arg1] << (8 - registers[arg2]);
			pcIncrement = 4;
			break;
		case (uint8_t)0x15:
			// shrl
			// Rotate shifts Arg1 left by Arg2 and saves to Arg3
			registers[arg3] = registers[arg1] << registers[arg2];
			registers[arg3] |= registers[arg1] >> (8 - registers[arg2]);
			pcIncrement = 4;
			break;



			// ALU::CONDITION



		case (uint8_t)0x18:
			// equ
			// Saves 255 if Arg1 == Arg2, 0 if not, to Arg3
			if (registers[arg1] == registers[arg2])
			{
				registers[arg3] = 255;
			}
			else
			{
				registers[arg3] = 0;
			}
			pcIncrement = 4;
			break;
		case (uint8_t)0x19:
			// nequ
			// Saves 255 if Arg1 != Arg2, 0 if not, to Arg3
			if (registers[arg1] != registers[arg2])
			{
				registers[arg3] = 255;
			}
			else
			{
				registers[arg3] = 0;
			}
			pcIncrement = 4;
			break;
		case (uint8_t)0x1a:
			// ules
			// Saves 255 if Arg1 < Arg2, 0 if not, to Arg3, treated as unsigned
			if (registers[arg1] < registers[arg2])
			{
				registers[arg3] = 255;
			}
			else
			{
				registers[arg3] = 0;
			}
			pcIncrement = 4;
			break;
		case (uint8_t)0x1b:
			// ulee
			// Saves 255 if Arg1 <= Arg2, 0 if not, to Arg3, treated as unsigned
			if (registers[arg1] <= registers[arg2])
			{
				registers[arg3] = 255;
			}
			else
			{
				registers[arg3] = 0;
			}
			pcIncrement = 4;
			break;
		case (uint8_t)0x1c:
			// sles
			// Saves 255 if Arg1 < Arg2, 0 if not, to Arg3, treated as signed
			if ((int8_t)registers[arg1] < (int8_t)registers[arg2])
			{
				registers[arg3] = 255;
			}
			else
			{
				registers[arg3] = 0;
			}
			pcIncrement = 4;
			break;
		case (uint8_t)0x1d:
			// slee
			// Saves 255 if Arg1 <= Arg2, 0 if not, to Arg3, treated as signed
			if ((int8_t)registers[arg1] <= (int8_t)registers[arg2])
			{
				registers[arg3] = 255;
			}
			else
			{
				registers[arg3] = 0;
			}
			pcIncrement = 4;
			break;



			// CONTROL



		case (uint8_t)0x20:
			// imm
			// Copies the value of Arg1 into Arg2
			registers[arg2] = arg1;
			pcIncrement = 3;
			break;
		case (uint8_t)0x21:
			// jmp
			// Jumps to the address stored in Arg1 if Arg2 != 0
			if (registers[arg2] != 0)
			{
				pc = registers[arg1];
                pcIncrement = 0;
			}
			else
			{
				pcIncrement = 3;
			}
			break;
		case (uint8_t)0x22:
			// jmpi
			// Jumps to the address given in Arg1 if Arg2 != 0. Above but with an immediate address
			if (registers[arg2] != 0)
			{
				pc = arg1;
                pcIncrement = 0;
			}
			else
			{
				pcIncrement = 3;
			}
			break;



			// MEMORY



		case (uint8_t)0x40:
		{
			// rme
			// Read address Arg1 from memory, saving into Arg2
			uint8_t cacheAddress = 0;
			uint8_t targetPageAddress = registers[arg1] & 0b111000000;
			uint8_t cacheHit = 0;
			for (uint8_t page = 0; page < memory.pageCount; page++)
			{
				if (targetPageAddress == memory.pages[page].address && memory.pages[page].valid == 1)
				{
					cacheAddress = page;
					cacheHit = 1;
				}
			}
			if (cacheHit == 0)
			{
				// cache miss
				uint8_t invalidPageFound = 0;
				for (uint8_t page = 0; page < memory.pageCount; page++)
				{
					if (memory.pages[page].valid == 1)
					{
						cacheAddress = page;
						invalidPageFound = 1;
					}
				}
				if (invalidPageFound == 0)
				{
					// cache flush
					cacheAddress = memory.leastRecent;
					if (memory.pages[cacheAddress].dirty == 1)
					{
						// write to ram
						for (uint8_t index = 0; index < 32; index++)
						{
							uint8_t ramAddress = index | memory.pages[cacheAddress].address;
							uint8_t copyCacheAddress = index | (cacheAddress << 5);
							ram[ramAddress] = cache[copyCacheAddress];
						}
					}
				}
				// copy new page to cache
				for (uint8_t index = 0; index < 32; index++)
				{
					uint8_t ramAddress = index | targetPageAddress;
					uint8_t copyCacheAddress = index | (cacheAddress << 5);
					cache[copyCacheAddress] = ram[ramAddress];
					memory.pages[cacheAddress].valid = 1;
					memory.pages[cacheAddress].dirty = 0;
				}
			}
			// read byte from cache
			cacheAddress = (cacheAddress << 5) | (registers[arg1] & 0b00011111);
			registers[arg2] = cache[cacheAddress];

			// update least recent
			if (memory.leastRecent == 0)
			{
				memory.leastRecent = 1;
			}
			else
			{
				memory.leastRecent = 0;
			}
			pcIncrement = 3;
		}
		break;
		case (uint8_t)0x41:
		{
			// wme
			// Write Arg2 to memory, saving at address Arg1
			uint8_t writeCacheAddress = 0;
			uint8_t writeTargetPageAddress = registers[arg1] & 0b111000000;
			uint8_t writeCacheHit = 0;
			for (uint8_t page = 0; page < memory.pageCount; page++)
			{
				if (writeTargetPageAddress == memory.pages[page].address && memory.pages[page].valid == 1)
				{
					writeCacheAddress = page;
					writeCacheHit = 1;
				}
			}
			if (writeCacheHit == 0)
			{
				// cache miss
				uint8_t invalidPageFound = 0;
				for (uint8_t page = 0; page < memory.pageCount; page++)
				{
					if (memory.pages[page].valid == 1)
					{
						writeCacheAddress = page;
						invalidPageFound = 1;
					}
				}
				if (invalidPageFound == 0)
				{
					// cache flush
					writeCacheAddress = memory.leastRecent;
					if (memory.pages[writeCacheAddress].dirty == 1)
					{
						// write to ram
						for (uint8_t index = 0; index < 32; index++)
						{
							uint8_t ramAddress = index | memory.pages[writeCacheAddress].address;
							uint8_t copywriteCacheAddress = index | (writeCacheAddress << 5);
							ram[ramAddress] = cache[copywriteCacheAddress];
						}
					}
				}
				// copy new page to cache
				for (uint8_t index = 0; index < 32; index++)
				{
					uint8_t ramAddress = index | writeTargetPageAddress;
					uint8_t copywriteCacheAddress = index | (writeCacheAddress << 5);
					cache[copywriteCacheAddress] = ram[ramAddress];
					memory.pages[writeCacheAddress].valid = 1;
					memory.pages[writeCacheAddress].dirty = 0;
				}
			}
			// write byte to cache
			memory.pages[writeCacheAddress].dirty = 1;
			writeCacheAddress = (writeCacheAddress << 5) | (registers[arg1] & 0b00011111);
			cache[writeCacheAddress] = registers[arg2];

			// update least recent
			if (memory.leastRecent == 0)
			{
				memory.leastRecent = 1;
			}
			else
			{
				memory.leastRecent = 0;
			}
			pcIncrement = 3;
		}
		break;
		default:
			printf("Unknown opcode\n");
			core_dump();
			return -1;
		}
        if ((uint8_t)(pc + pcIncrement) < pc)
        {
            break;
        }
        pc += pcIncrement;
	}
	printf("PC reached end of program cache\n");
	core_dump();
	return 0;
}

void core_dump()
{
	printf("CORE DUMP\n");

	// print the contents of the program memory
	printf("PROGRAM:\n");
	printf("0x _0 _1 _2 _3 _4 _5 _6 _7 _8 _9 _a _b _c _d _e _f\n");
	for (int row = 0; row < 16; row++)
	{
		printf("%01x_ ", row & 0xf);
		for (int cell = 0; cell < 16; cell++)
		{
			printf("%02x ", program[row * 16 + cell] & 0xff);
		}
		printf("\n");
	}

	// print the contents of the registers
	printf("  pc = 0x%02x\n", pc & 0xff);
	for (uint8_t reg = 0; reg < 8; reg++)
	{
		printf("reg%01x = 0x%02x\n", reg & 0xff, registers[reg] & 0xff);
	}

    // print the contents of the MMU
    printf("MMU:\n");
    printf("| Least recent page accessed = 0x%02x\n", memory.leastRecent & 0xff);
    printf("| Page count = 0x%02x\n", memory.pageCount & 0xff);
    for (int page = 0; page < memory.pageCount; page++)
    {
        printf("|-Page number 0x%02x\n", page & 0xff);
        printf("| |-Valid = %d\n", memory.pages[page].valid);
        printf("| |-Address = 0x%02x\n", memory.pages[page].address & 0xff);
        printf("| |-Dirty = %d\n", memory.pages[page].dirty);
    }

	// print the contents of the cache
	printf("CACHE:\n");
	printf("0x _0 _1 _2 _3 _4 _5 _6 _7 _8 _9 _a _b _c _d _e _f\n");
	for (int row = 0; row < 4; row++)
	{
		printf("%01x_ ", row & 0xf);
		for (int cell = 0; cell < 16; cell++)
		{
			printf("%02x ", cache[row * 16 + cell] & 0xff);
		}
		printf("\n");
	}

	// print the contents of the ram
	printf("RAM:\n");
	printf("0x _0 _1 _2 _3 _4 _5 _6 _7 _8 _9 _a _b _c _d _e _f\n");
	for (int row = 0; row < 16; row++)
	{
		printf("%01x_ ", row & 0xf);
		for (int cell = 0; cell < 16; cell++)
		{
			printf("%02x ", ram[row * 16 + cell] & 0xff);
		}
		printf("\n");
	}

}
