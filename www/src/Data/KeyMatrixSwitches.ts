import { useEffect, useState } from 'react';
import WebApi from '../Services/WebApi';

// Input pins the firmware tracks: the real GPIOs plus keyboard matrix switches.
export const NUM_INPUT_PINS = 64;

export type MatrixSwitch = {
	pin: number;
	row: number;
	col: number;
};

export const matrixSwitchName = ({ row, col }: MatrixSwitch) =>
	`R${row}C${col}`;

// one request per page load, shared by every user of the hook
let switchesRequest: Promise<MatrixSwitch[]> | null = null;
const fetchSwitches = () => {
	if (!switchesRequest) {
		switchesRequest = WebApi.getKeyMatrixOptions().then((data) => {
			if (!data?.enabled) return [];
			const cols = Math.max(1, data.colPins?.length ?? 1);
			const pins: number[] = data.switchPins ?? [];
			return pins.map((pin, index) => ({
				pin,
				row: Math.floor(index / cols) + 1,
				col: (index % cols) + 1,
			}));
		});
	}
	return switchesRequest;
};

// The keyboard matrix switches of the running firmware, with the input pin each
// one uses. Empty when the matrix is off.
export const useKeyMatrixSwitches = () => {
	const [switches, setSwitches] = useState<MatrixSwitch[]>([]);
	useEffect(() => {
		let active = true;
		fetchSwitches().then((result) => {
			if (active) setSwitches(result);
		});
		return () => {
			active = false;
		};
	}, []);
	return switches;
};
