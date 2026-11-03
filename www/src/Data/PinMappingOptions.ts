import invert from 'lodash/invert';
import { MultiValue, SingleValue } from 'react-select';

import { BUTTON_MASKS, DPAD_MASKS } from './Buttons';
import { BUTTON_ACTIONS, PinActionValues } from './Pins';
import { MaskPayload } from '../Store/useProfilesStore';

// Pin action options shared by the pin mapping page and the keyboard matrix
// add-on. Several buttons picked together make a custom button combo.

export type OptionType = {
	label: string;
	value: PinActionValues;
	type: string;
	customButtonMask: number;
	customDpadMask: number;
};

const disabledOptions = [
	BUTTON_ACTIONS.RESERVED,
	BUTTON_ACTIONS.ASSIGNED_TO_ADDON,
] as PinActionValues[];

const getMask = (maskArr: { label: string; value: number }[], key: string) =>
	maskArr.find(
		({ label }) => label?.toUpperCase() === key.split('BUTTON_PRESS_')?.pop(),
	);

const isNonSelectable = (action: PinActionValues) =>
	[
		BUTTON_ACTIONS.NONE,
		BUTTON_ACTIONS.CUSTOM_BUTTON_COMBO,
		...disabledOptions,
	].includes(action);

export const isDisabled = (action: PinActionValues) =>
	disabledOptions.includes(action);

export const options = Object.entries(BUTTON_ACTIONS)
	.filter(([, value]) => !isNonSelectable(value))
	.map(([key, value]) => {
		const buttonMask = getMask(BUTTON_MASKS, key);
		const dpadMask = getMask(DPAD_MASKS, key);

		return {
			label: key,
			value,
			type: buttonMask
				? 'customButtonMask'
				: dpadMask
					? 'customDpadMask'
					: 'action',
			customButtonMask: buttonMask?.value || 0,
			customDpadMask: dpadMask?.value || 0,
		};
	});

export const groupedOptions = [
	{
		label: 'Buttons',
		options: options.filter(({ type }) => type !== 'action'),
	},
	{
		label: 'Actions',
		options: options.filter(({ type }) => type === 'action'),
	},
];

export const getMultiValue = (pinData: MaskPayload) => {
	if (pinData.action === BUTTON_ACTIONS.NONE) return;
	if (isDisabled(pinData.action)) {
		const actionKey = invert(BUTTON_ACTIONS)[pinData.action];
		return [
			{
				label: actionKey,
				value: pinData.action,
				type: 'action',
				customButtonMask: pinData.customButtonMask,
				customDpadMask: pinData.customDpadMask,
			},
		];
	}

	return pinData.action === BUTTON_ACTIONS.CUSTOM_BUTTON_COMBO
		? options.filter(
				({ type, customButtonMask, customDpadMask }) =>
					(pinData.customButtonMask & customButtonMask &&
						type === 'customButtonMask') ||
					(pinData.customDpadMask & customDpadMask &&
						type === 'customDpadMask'),
			)
		: options.filter((option) => option.value === pinData.action);
};

export const selectionToMapping = (
	selected: MultiValue<OptionType> | SingleValue<OptionType>,
): MaskPayload => {
	// Handle clearing
	if (!selected || (Array.isArray(selected) && !selected.length)) {
		return {
			action: BUTTON_ACTIONS.NONE,
			customButtonMask: 0,
			customDpadMask: 0,
		};
	}
	if (Array.isArray(selected) && selected.length > 1) {
		const lastSelected = selected[selected.length - 1];
		// Revert to single option if choosing action type
		if (lastSelected.type === 'action') {
			return {
				action: lastSelected.value,
				customButtonMask: 0,
				customDpadMask: 0,
			};
		}
		return selected.reduce(
			(masks, option) => ({
				...masks,
				customButtonMask:
					option.type === 'customButtonMask'
						? masks.customButtonMask ^ option.customButtonMask
						: masks.customButtonMask,
				customDpadMask:
					option.type === 'customDpadMask'
						? masks.customDpadMask ^ option.customDpadMask
						: masks.customDpadMask,
			}),
			{
				action: BUTTON_ACTIONS.CUSTOM_BUTTON_COMBO,
				customButtonMask: 0,
				customDpadMask: 0,
			} as MaskPayload,
		);
	}
	const single = Array.isArray(selected) ? selected[0] : selected;
	return {
		action: single.value,
		customButtonMask: 0,
		customDpadMask: 0,
	};
};
