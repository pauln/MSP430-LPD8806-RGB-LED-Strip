#define SRAM_HOLD	BIT3
#define SRAM_CLK	BIT4
#define SRAM_OUT	BIT5
#define SRAM_CS		BIT1
#define SRAM_IN		BIT2
#define SRAM_WRSR	0x01
#define SRAM_WRITE	0x02
#define SRAM_READ	0x03
#define SRAM_RDSR	0x05

long sram_bit;
long sram_byte;

void sram_select_chip() {
	P2OUT &= ~SRAM_OUT;
	P2OUT &= ~SRAM_CLK;
	P2OUT &= ~SRAM_CS;
}
void sram_deselect_chip() {
	P2OUT |= SRAM_CS;
}
void sram_hold() {
	P2OUT &= ~SRAM_HOLD;
}
void sram_unhold() {
	P2OUT |= SRAM_HOLD;
}

void sram_send(unsigned int mask, unsigned int data) {
	for ( sram_bit=mask; sram_bit>0; sram_bit>>=1 ) {
		P2OUT &= ~SRAM_CLK;
		if (data & sram_bit) {
			P2OUT |= SRAM_OUT;
		} else {
			P2OUT &= ~SRAM_OUT;
		}
		P2OUT |= SRAM_CLK;
	}
	P2OUT &= ~SRAM_CLK; // reset clock to low so we know it'll be low when we try to do something else
}

void sram_send_command(unsigned char command) {
	sram_send(0x80, command);
}

void sram_send_address(unsigned int address) {
	sram_send(0x8000, address);
}

void sram_set_status(unsigned char status) {
	sram_select_chip();

	sram_send_command(SRAM_WRSR);
	sram_send(0x80, status);

	sram_deselect_chip();
}

unsigned char sram_get_status() {
	unsigned char data = 0x00;
	sram_select_chip();

	sram_send_command(SRAM_RDSR);
	for ( sram_bit=0x80; sram_bit>0; sram_bit>>=1 ) {
		P2OUT |= SRAM_CLK;
		if (P2IN & SRAM_IN) {
			data |= SRAM_IN;
		}
		P2OUT &= ~SRAM_CLK;
	}

	sram_deselect_chip();

	return data;
}

void sram_init() {
	P2DIR |= SRAM_HOLD + SRAM_CLK + SRAM_OUT + SRAM_CS; // Set hold, clock, data out, chip select as outputs
	P2DIR &= ~SRAM_IN; // Set data in as input
	P2OUT &= ~SRAM_CLK; // SRAM reads on rising edge, so be prepared; also must be low when HOLD rises
	P2OUT &= ~SRAM_OUT; // Initialise output to 0, so we know where it is
	P2OUT &= ~SRAM_HOLD; // Initialise output to 0, so we can pull it high with CLK low
	P2OUT |= SRAM_CS + SRAM_HOLD; // Pull CS high to disable the chip, but pull HOLD high so it's not paused
	sram_set_status(0x40);
}

void sram_write_bytes(unsigned int address, unsigned char *data, long num_bytes) {
	sram_select_chip();

	sram_send_command(SRAM_WRITE);
	sram_send_address(address);

	// Output data
	for ( sram_byte=0; sram_byte < num_bytes; sram_byte++ ) {
		sram_send(0x80, data[sram_byte]);
	}

	sram_deselect_chip();
}

void sram_write_byte(unsigned int address, unsigned char byte) {
	unsigned char data[1];
	data[0] = byte;

	sram_write_bytes(address, data, 1);
}

void sram_read_bytes(unsigned int address, unsigned char *data, long num_bytes) {
	sram_select_chip();

	sram_send_command(SRAM_READ);
	sram_send_address(address);

	// Read data
	for ( sram_byte=0; sram_byte < num_bytes; sram_byte++ ) {
		for ( sram_bit=0x80; sram_bit>0; sram_bit>>=1 ) {
			P2OUT |= SRAM_CLK;
			if (P2IN & SRAM_IN) {
				data[sram_byte] |= sram_bit;
			} else {
				data[sram_byte] &= ~sram_bit;
			}
			P2OUT &= ~SRAM_CLK;
		}
	}

	sram_deselect_chip();
}

void sram_start_read(unsigned int address, unsigned char *data, long num_bytes) {
	sram_select_chip();

	sram_send_command(SRAM_READ);
	sram_send_address(address);

	// Read data
	for ( sram_byte=0; sram_byte < num_bytes; sram_byte++ ) {
		for ( sram_bit=0x80; sram_bit>0; sram_bit>>=1 ) {
			P2OUT |= SRAM_CLK;
			if (P2IN & SRAM_IN) {
				data[sram_byte] |= sram_bit;
			} else {
				data[sram_byte] &= ~sram_bit;
			}
			P2OUT &= ~SRAM_CLK;
		}
	}

	sram_hold();
}

void sram_continue_read(unsigned char *data, long num_bytes) {
	sram_unhold();

	// Read data
	for ( sram_byte=0; sram_byte < num_bytes; sram_byte++ ) {
		for ( sram_bit=0x80; sram_bit>0; sram_bit>>=1 ) {
			P2OUT |= SRAM_CLK;
			if (P2IN & SRAM_IN) {
				data[sram_byte] |= sram_bit;
			} else {
				data[sram_byte] &= ~sram_bit;
			}
			P2OUT &= ~SRAM_CLK;
		}
	}

	sram_hold();
}

void sram_finish_read() {
	sram_unhold();
	sram_deselect_chip();
}

unsigned char sram_read_byte(unsigned int address) {
	unsigned char data[1];

	sram_read_bytes(address, data, 1);

	return data[0];
}