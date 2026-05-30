import { describe, expect, it } from 'vitest';
import {
	canonicalizeFileManagerPath,
	isFileManagerReadOnlyPath,
	validateFileManagerPathAccess
} from './utils';

describe('fileManager utils', () => {
	it('canonicalizes duplicate slashes and relative roots', () => {
		expect(canonicalizeFileManagerPath('data//logs/')).toEqual({
			ok: true,
			path: '/data/logs'
		});
	});

	it('rejects traversal attempts', () => {
		expect(canonicalizeFileManagerPath('/littlefs/../config')).toEqual({
			ok: false,
			error: 'fs/invalid_path'
		});
		expect(canonicalizeFileManagerPath('/foo/..')).toEqual({
			ok: false,
			error: 'fs/invalid_path'
		});
	});

	it('keeps protected config paths visible for read operations', () => {
		expect(canonicalizeFileManagerPath('/littlefs/config')).toEqual({
			ok: true,
			path: '/littlefs/config'
		});
		expect(isFileManagerReadOnlyPath('/littlefs/config')).toBe(true);
		expect(validateFileManagerPathAccess('/littlefs/config', 'list')).toEqual({
			ok: true,
			path: '/littlefs/config'
		});
		expect(validateFileManagerPathAccess('/littlefs/config', 'download')).toEqual({
			ok: true,
			path: '/littlefs/config'
		});
	});

	it('blocks writes to protected config paths', () => {
		expect(validateFileManagerPathAccess('//config', 'upload')).toEqual({
			ok: false,
			error: 'fs/path_forbidden'
		});
		expect(validateFileManagerPathAccess('/littlefs/config', 'remove')).toEqual({
			ok: false,
			error: 'fs/path_forbidden'
		});
	});

	it('uses the provided fallback for empty directories', () => {
		expect(canonicalizeFileManagerPath('', '/sdcard')).toEqual({
			ok: true,
			path: '/sdcard'
		});
	});
});
