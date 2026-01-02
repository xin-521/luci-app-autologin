#!/usr/bin/ucode

import * as uci from 'uci';
import * as fs from 'fs';
import * as subprocess from 'subprocess';

const cursor = uci.cursor();

const methods = {
	getStatus: function() {
		const enabled = cursor.get('autologin', 'config', 'enabled') || '0';
		let status = 'Disabled';

		if (enabled === '1') {
			const proc = subprocess.exec([ 'pgrep', '-f', 'autologin' ], { stdout: 'pipe' });
			if (proc.stdout && proc.stdout.trim() !== '') {
				status = 'Running';
			} else {
				status = 'Stopped';
			}
		}

		return { status: status };
	},

	doLogin: function() {
		const username = cursor.get('autologin', 'config', 'username') || '';
		const password = cursor.get('autologin', 'config', 'password') || '';
		const domain = cursor.get('autologin', 'config', 'domain') || '';
		const server = cursor.get('autologin', 'config', 'server') || '172.26.8.11';

		if (username === '' || password === '') {
			return {
				success: false,
				message: 'Username or password not configured'
			};
		}

		const cmd = [ '/usr/bin/autologin', username, password, domain, server ];
		const proc = subprocess.exec(cmd, { stdout: 'pipe', stderr: 'pipe' });

		return {
			success: proc.returncode === 0,
			message: proc.stdout || proc.stderr || 'Login attempt completed'
		};
	}
};

return methods;
