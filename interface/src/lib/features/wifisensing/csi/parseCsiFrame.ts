export interface CsiAmplitudeBuffers {
	bufferA: Float32Array;
	bufferB: Float32Array;
	flip: boolean;
}

export interface ParsedCsiFrame {
	timestamp: number;
	rssi: number;
	gain: number;
	subcarriers: number;
	amplitudes: Float32Array;
	motionScore: number;
	isMotionDetected: boolean;
	buffers: CsiAmplitudeBuffers;
}

const CSI_HEADER_BYTES = 13;

export function parseCsiFrame(
	buffer: ArrayBuffer,
	currentBuffers: CsiAmplitudeBuffers
): ParsedCsiFrame | null {
	const view = new DataView(buffer);
	if (view.byteLength < CSI_HEADER_BYTES) return null;

	let offset = 0;
	const timestamp = view.getUint32(offset, true);
	offset += 4;

	const rssi = view.getInt8(offset);
	offset += 1;

	const dataLength = view.getUint16(offset, true);
	offset += 2;

	const gain = view.getUint8(offset) / 10.0;
	offset += 1;

	const motionScore = view.getFloat32(offset, true);
	offset += 4;

	const isMotionDetected = view.getUint8(offset) === 1;
	offset += 1;

	if (dataLength === 0 || dataLength % 2 !== 0) return null;
	if (view.byteLength < offset + dataLength) return null;

	const subcarriers = dataLength / 2;
	let bufferA = currentBuffers.bufferA;
	let bufferB = currentBuffers.bufferB;

	if (bufferA.length !== subcarriers) {
		bufferA = new Float32Array(subcarriers);
		bufferB = new Float32Array(subcarriers);
	}

	const amplitudes = currentBuffers.flip ? bufferA : bufferB;
	for (let index = 0; index < subcarriers; index++) {
		const real = view.getInt8(offset + index * 2);
		const imag = view.getInt8(offset + index * 2 + 1);
		amplitudes[index] = Math.sqrt(real * real + imag * imag);
	}

	return {
		timestamp,
		rssi,
		gain,
		subcarriers,
		amplitudes,
		motionScore,
		isMotionDetected,
		buffers: {
			bufferA,
			bufferB,
			flip: !currentBuffers.flip
		}
	};
}
