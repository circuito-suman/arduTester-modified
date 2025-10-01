/*
 * I2C Scanner for ArduTester
 * Scans I2C addresses from 0x08 to 0x77 and displays found devices
 * Uses Arduino Nano's A4 (SDA) and A5 (SCL) pins
 */

#ifdef WITH_MENU

void I2C_Scanner() {
  uint8_t error, address;
  uint8_t found_addresses[112]; // Maximum possible I2C addresses
  uint8_t device_count = 0;
  uint8_t current_page = 0;
  uint8_t devices_per_page = 6; // Display up to 6 addresses (3 per line, 2 lines)
  uint8_t total_pages;
  uint8_t ii;

  lcd_clear();
  lcd_line1();
  lcd_MEM2_string(I2C_SCAN_str); // "I2C Scanner"
  lcd_line2();
  lcd_MEM2_string(Scanning_str); // "Scanning..."
  lcd_refresh();

  // NOTE: Buzzer sounds are disabled during I2C scanning because
  // the buzzer uses A5 which is the same pin as I2C SCL
  // Play scan start sound BEFORE initializing I2C
  BUZZER_I2C_SCAN_START();

  // Small delay to let the buzzer finish
  sleep_5ms(20); // Wait 100ms (20 * 5ms)

  // Initialize I2C - using simple bit-bang I2C on A4/A5
  // Set A4 (SDA) and A5 (SCL) as inputs with pullup
  ADC_DDR &= ~((1<<4)|(1<<5));  // Set A4 and A5 as inputs
  ADC_PORT |= (1<<4)|(1<<5);    // Enable pullups on A4 and A5

  // Let the bus stabilize
  wait_about1s();

  // Scan from address 0x08 to 0x77 (standard I2C address range)
  for(address = 0x08; address <= 0x77; address++) {
    // Simple I2C device detection
    if (i2c_test_device(address)) {
      found_addresses[device_count] = address;
      device_count++;

      // NOTE: Device-specific sounds are disabled during scanning
      // because A5 is being used for I2C SCL (conflicts with buzzer)
      // We'll play sounds after scanning is complete
    }
  }

  // Restore A5 pin for buzzer use after I2C scanning
  // Set A5 back to normal I/O mode 
  ADC_DDR &= ~(1<<5);   // A5 as input
  ADC_PORT &= ~(1<<5);  // Disable pullup on A5

  // Display results
  if (device_count == 0) {
    lcd_clear();
    lcd_line1();
    lcd_MEM2_string(NoI2C_str); // "No I2C devices"
    lcd_line2();
    lcd_MEM2_string(Found_str); // "found!"
    lcd_refresh();

    // Now we can play buzzer sounds again (A5 is free)
    BUZZER_I2C_NO_DEVICES();

    wait_for_key_ms(3000);
  } else {
    // Calculate total pages needed
    total_pages = (device_count + devices_per_page - 1) / devices_per_page;

    // Play success sound now that A5 is free for buzzer
    BUZZER_I2C_FOUND();

    // Display first page
    display_i2c_page(found_addresses, device_count, current_page, devices_per_page);

    // If multiple pages, allow navigation
    if (total_pages > 1) {
      while(1) {
        ii = wait_for_key_ms(3000);

        if (ii == 0) break; // Timeout, exit

        // Next page on key press
        current_page++;
        if (current_page >= total_pages) {
          current_page = 0; // Loop back to first page
        }

        display_i2c_page(found_addresses, device_count, current_page, devices_per_page);
      }
    } else {
      // Single page, just wait for key press or timeout
      wait_for_key_ms(3000);
    }
  }

  // Wait for key release before returning to menu
  while(!(RST_PIN_REG & (1<<RST_PIN)));
}

void display_i2c_page(uint8_t* addresses, uint8_t total_devices, uint8_t page, uint8_t per_page) {
  uint8_t start_idx = page * per_page;
  uint8_t end_idx = start_idx + per_page;
  uint8_t i, addr_on_line;

  if (end_idx > total_devices) {
    end_idx = total_devices;
  }

  lcd_clear();
  lcd_line1();

  // Show device count and page info
  u2lcd(total_devices);
  lcd_MEM2_string(I2C_Found_str2); // " I2C found"

  uint8_t total_pages = (total_devices + per_page - 1) / per_page;
  if (total_pages > 1) {
    lcd_MEM2_string(Page_str); // " P"
    u2lcd(page + 1);
    lcd_data('/');
    u2lcd(total_pages);
  }

  // Display addresses - 3 per line, up to 2 lines
  lcd_line2();
  addr_on_line = 0;

  for (i = start_idx; i < end_idx; i++) {
    if (addr_on_line == 3) {
      // Move to next line after 3 addresses
#if FLASHEND > 0x1fff
      lcd_line3();
#endif
      addr_on_line = 0;
    }

    if (addr_on_line > 0) {
      lcd_space(); // Space between addresses
    }

    lcd_MEM2_string(Addr_str); // "0x"
    if (addresses[i] < 0x10) lcd_data('0'); // Leading zero for single digit hex

    // Convert to hex and display
    uint8_t high_nibble = (addresses[i] >> 4) & 0x0F;
    uint8_t low_nibble = addresses[i] & 0x0F;

    if (high_nibble < 10) lcd_data('0' + high_nibble);
    else lcd_data('A' + high_nibble - 10);

    if (low_nibble < 10) lcd_data('0' + low_nibble);
    else lcd_data('A' + low_nibble - 10);

    addr_on_line++;
  }

  lcd_refresh();
}

// Robust I2C device test function
uint8_t i2c_test_device(uint8_t address) {
  uint8_t ack1 = 0, ack2 = 0, ack3 = 0;
  uint8_t test_count = 0;
  uint8_t i;

  // First, test if the I2C bus lines are properly pulled up
  // If not, we can't have valid I2C communication
  ADC_DDR &= ~((1<<4)|(1<<5));  // Both pins as inputs with pullups
  ADC_PORT |= (1<<4)|(1<<5);    // Enable pullups
  wait5us();

  // Check if both lines are high (proper pullups)
  if (((ADC_PIN & (1<<4)) == 0) || ((ADC_PIN & (1<<5)) == 0)) {
    // Lines are not high - no proper I2C bus
    return 0;
  }

  // Test multiple times to avoid false positives from noise
  for (i = 0; i < 3; i++) {
    // Start condition
    i2c_start();

    // Send address with write bit
    ack1 = i2c_write_byte((address << 1));

    // Stop condition
    i2c_stop();

    // Small delay between tests
    wait5us();
    wait5us();

    if (ack1) {
      test_count++;
    }
  }

  // Device must ACK consistently (at least 2 out of 3 times)
  if (test_count < 2) {
    return 0; // Inconsistent response, likely noise
  }

  // Final validation: try to communicate with the device
  i2c_start();
  ack2 = i2c_write_byte((address << 1)); // Send address

  if (ack2) {
    // Try to write a register address (0x00 is usually safe)
    ack3 = i2c_write_byte(0x00);
    // Note: We don't require ack3 to be true, as some devices
    // might not have register 0x00 or might NACK data
  }

  i2c_stop();

  // Return true only if we got consistent address ACKs
  return (ack2 && (test_count >= 2));
}

// I2C start condition
void i2c_start(void) {
  // SDA high, SCL high
  ADC_DDR &= ~((1<<4)|(1<<5));
  wait5us();

  // SDA low while SCL high (start condition)
  ADC_DDR |= (1<<4);  // SDA as output (low)
  wait5us();

  // SCL low
  ADC_DDR |= (1<<5);  // SCL as output (low)
  wait5us();
}

// I2C stop condition
void i2c_stop(void) {
  // SCL low, SDA low
  ADC_DDR |= (1<<4)|(1<<5);
  wait5us();

  // SCL high
  ADC_DDR &= ~(1<<5);
  wait5us();

  // SDA high (stop condition)
  ADC_DDR &= ~(1<<4);
  wait5us();
}

// I2C write byte and return ACK status
uint8_t i2c_write_byte(uint8_t data) {
  uint8_t i;
  uint8_t ack;
  uint8_t sda_before, sda_during, sda_after;

  // Send 8 bits
  for (i = 0; i < 8; i++) {
    // Set SDA according to bit
    if (data & 0x80) {
      ADC_DDR &= ~(1<<4); // SDA high (input with pullup)
    } else {
      ADC_DDR |= (1<<4);  // SDA low (output)
    }
    wait5us();

    // Clock pulse
    ADC_DDR &= ~(1<<5); // SCL high
    wait5us();
    ADC_DDR |= (1<<5);  // SCL low
    wait5us();

    data <<= 1;
  }

  // Enhanced ACK detection with multiple reads
  ADC_DDR &= ~(1<<4); // SDA as input
  wait5us();

  // Read SDA state before clock (should be high from pullup)
  sda_before = (ADC_PIN & (1<<4));

  ADC_DDR &= ~(1<<5); // SCL high
  wait5us();

  // Read SDA multiple times during ACK period for stability
  sda_during = (ADC_PIN & (1<<4));
  wait5us();

  // Read again to ensure stable signal
  sda_after = (ADC_PIN & (1<<4));

  ADC_DDR |= (1<<5);  // SCL low
  wait5us();

  // Valid ACK requires:
  // 1. SDA was high before (from pullup)
  // 2. SDA is consistently low during ACK period (both reads)
  ack = (sda_before != 0) && (sda_during == 0) && (sda_after == 0);

  return ack;
}

#endif // WITH_MENU