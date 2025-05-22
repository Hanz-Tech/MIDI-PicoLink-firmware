/**
 * Web Serial handler for RP2040 configurator.
 * Usage:
 *   await serialHandler.init();
 *   await serialHandler.write(jsonString);
 *   const response = await serialHandler.read();
 */

export class SerialHandler {
  reader: ReadableStreamDefaultReader | null = null;
  writer: WritableStreamDefaultWriter | null = null;
  port: SerialPort | null = null;
  textDecoder: TextDecoderStream = new TextDecoderStream();
  textEncoder: TextEncoderStream = new TextEncoderStream();
  readableStreamClosed: Promise<void> | null = null;
  writableStreamClosed: Promise<void> | null = null;

  async init(): Promise<boolean> {
    if ('serial' in navigator) {
      try {
        // Try to auto-select a previously used port, else prompt
        const ports = (await navigator.serial.getPorts());
        if (ports.length >= 1) {
          this.port = ports[0];
        }
        if (!this.port) {
          this.port = await navigator.serial.requestPort();
        }
        await this.port.open({ baudRate: 115200 });
        if (this.port.writable != null) {
          this.writableStreamClosed = this.textEncoder.readable.pipeTo(this.port.writable);
          this.writer = this.textEncoder.writable.getWriter();
        } else {
          throw Error("Port is not writable");
        }
        if (this.port.readable != null) {
          this.readableStreamClosed = this.port.readable.pipeTo(this.textDecoder.writable);
          this.reader = this.textDecoder.readable.getReader();
        } else {
          throw Error("Port is not readable");
        }
        return true;
      } catch (err) {
        console.error('Error opening serial port:', err);
        throw err;
      }
    } else {
      alert('Web Serial API not supported in this browser.');
      return false;
    }
  }

  async write(data: string): Promise<void> {
    if (!this.writer) throw new Error("Serial port not open");
    await this.writer.write(data);
  }

  async readLine(): Promise<string> {
    if (!this.reader) throw new Error("Serial port not open");
    let buffer = "";
    while (true) {
      const { value, done } = await this.reader.read();
      if (done) {
        this.reader.releaseLock();
        break;
      }
      buffer += value;
      const idx = buffer.indexOf('\n');
      if (idx !== -1) {
        const line = buffer.slice(0, idx);
        buffer = buffer.slice(idx + 1);
        return line.trim();
      }
    }
    return buffer.trim();
  }

  async close(): Promise<void> {
    if (this.writer) {
      await this.writer.close();
      this.writer = null;
    }
    if (this.reader) {
      await this.reader.cancel();
      this.reader = null;
    }
    if (this.port) {
      await this.port.close();
      this.port = null;
    }
  }
}

export const serialHandler = new SerialHandler();
