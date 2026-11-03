import { useContext, useEffect, useMemo, useState } from 'react';
import { Alert, Button, Col, Form, FormCheck, Row } from 'react-bootstrap';
import { useTranslation } from 'react-i18next';
import invert from 'lodash/invert';
import omit from 'lodash/omit';

import { AppContext } from '../Contexts/AppContext';
import Section from '../Components/Section';
import CustomSelect from '../Components/CustomSelect';
import WebApi from '../Services/WebApi';
import { getButtonLabels } from '../Data/Buttons';
import { BUTTON_ACTIONS } from '../Data/Pins';

const NONE = -10;
// Matrix switches are plain inputs: they can't be reserved, handed to an
// add-on or use a custom button combo (no room to store the combo masks).
const UNSELECTABLE_ACTIONS = [
	BUTTON_ACTIONS.RESERVED,
	BUTTON_ACTIONS.ASSIGNED_TO_ADDON,
	BUTTON_ACTIONS.CUSTOM_BUTTON_COMBO,
];
const NUM_INPUT_PINS = 64;

const actionOptions = Object.entries(BUTTON_ACTIONS)
	.filter(([, value]) => !UNSELECTABLE_ACTIONS.includes(value))
	.map(([label, value]) => ({ label, value }));

type KeyMatrixOptions = {
	enabled: boolean;
	settleMicros: number;
	firstVirtualPin: number;
	rowPins: number[];
	colPins: number[];
	pinActions: number[];
};

const parsePins = (text: string): number[] | null => {
	const parts = text
		.split(',')
		.map((part) => part.trim())
		.filter((part) => part.length > 0);
	if (parts.some((part) => !/^\d+$/.test(part))) return null;
	return parts.map((part) => parseInt(part, 10));
};

// Same order the firmware uses: the row/column GPIOs' own pin bits first,
// then the pins above the real GPIOs.
const getSwitchPins = (
	rowPins: number[],
	colPins: number[],
	firstVirtualPin: number,
) => {
	const matrixPins = new Set([...rowPins, ...colPins]);
	const wanted = rowPins.length * colPins.length;
	const pins: number[] = [];
	for (let pin = 0; pin < NUM_INPUT_PINS && pins.length < wanted; pin++) {
		if (pin >= firstVirtualPin || matrixPins.has(pin)) pins.push(pin);
	}
	return pins;
};

const KeyMatrix = () => {
	const { t } = useTranslation();
	const { buttonLabels, updateUsedPins } = useContext(AppContext);
	const [options, setOptions] = useState<KeyMatrixOptions | null>(null);
	const [rowText, setRowText] = useState('');
	const [colText, setColText] = useState('');
	const [message, setMessage] = useState('');

	const { buttonLabelType, swapTpShareLabels } = buttonLabels;
	const buttonNames = omit(
		getButtonLabels(buttonLabelType, swapTpShareLabels),
		['label', 'value'],
	);

	useEffect(() => {
		WebApi.getKeyMatrixOptions().then((data: KeyMatrixOptions) => {
			if (!data) return;
			setOptions(data);
			setRowText(data.rowPins.join(', '));
			setColText(data.colPins.join(', '));
		});
	}, []);

	const rowPins = parsePins(rowText);
	const colPins = parsePins(colText);
	const firstVirtualPin = options?.firstVirtualPin ?? 0;
	const switchPins = useMemo(
		() =>
			rowPins && colPins && firstVirtualPin > 0
				? getSwitchPins(rowPins, colPins, firstVirtualPin)
				: [],
		[rowText, colText, firstVirtualPin],
	);

	if (!options) return null;

	const setAction = (index: number, action: number) => {
		const pinActions = [...options.pinActions];
		pinActions[index] = action;
		setOptions({ ...options, pinActions });
	};

	const getActionLabel = (label: string) => {
		const key = label.split('BUTTON_PRESS_').pop();
		return (key && buttonNames[key]) || t(`Proto:GpioAction.${label}`);
	};

	const save = async () => {
		if (!rowPins || !colPins) {
			setMessage(t('KeyMatrix:invalid-pins'));
			return;
		}
		try {
			const response = await WebApi.setKeyMatrixOptions({
				...options,
				rowPins,
				colPins,
			});
			if (response?.data?.error) {
				setMessage(response.data.error);
				return;
			}
			setOptions({ ...options, rowPins, colPins });
			setMessage(t('Common:saved-success-message'));
			updateUsedPins();
		} catch (error) {
			setMessage(t('Common:saved-error-message'));
		}
	};

	const rows = rowPins?.length ?? 0;
	const cols = colPins?.length ?? 0;

	return (
		<Section title={t('KeyMatrix:header-text')}>
			<div hidden={!options.enabled}>
				<div className="alert alert-info" role="alert">
					{t('KeyMatrix:sub-header-text')}
				</div>
				<p>{t('KeyMatrix:pin-note', { firstVirtual: firstVirtualPin })}</p>
				<Row className="mb-3">
					<Form.Group as={Col} sm={4}>
						<Form.Label>{t('KeyMatrix:row-pins-label')}</Form.Label>
						<Form.Control
							size="sm"
							value={rowText}
							placeholder={t('KeyMatrix:pins-placeholder')}
							isInvalid={!rowPins}
							onChange={(e) => setRowText(e.target.value)}
						/>
					</Form.Group>
					<Form.Group as={Col} sm={4}>
						<Form.Label>{t('KeyMatrix:col-pins-label')}</Form.Label>
						<Form.Control
							size="sm"
							value={colText}
							placeholder={t('KeyMatrix:pins-placeholder')}
							isInvalid={!colPins}
							onChange={(e) => setColText(e.target.value)}
						/>
					</Form.Group>
					<Form.Group as={Col} sm={2}>
						<Form.Label>{t('KeyMatrix:settle-label')}</Form.Label>
						<Form.Control
							size="sm"
							type="number"
							min={1}
							max={1000}
							value={options.settleMicros}
							onChange={(e) =>
								setOptions({
									...options,
									settleMicros: parseInt(e.target.value, 10) || 1,
								})
							}
						/>
					</Form.Group>
				</Row>
				<p>
					{switchPins.length < rows * cols
						? t('KeyMatrix:switch-count-capped', {
								rows,
								cols,
								wanted: rows * cols,
								count: switchPins.length,
							})
						: t('KeyMatrix:switch-count', {
								rows,
								cols,
								count: switchPins.length,
							})}
				</p>
				<div
					className="d-grid gap-2 mb-3"
					style={{
						gridTemplateColumns: 'repeat(auto-fill, minmax(14rem, 1fr))',
					}}
				>
					{switchPins.map((pin, index) => {
						const action = options.pinActions[index] ?? NONE;
						return (
							<div
								key={`matrix-switch-${index}`}
								className="d-flex align-items-center gap-2"
							>
								<div className="flex-shrink-0 small" style={{ width: '5rem' }}>
									{t('KeyMatrix:switch-label', {
										row: Math.floor(index / cols) + 1,
										col: (index % cols) + 1,
									})}
									<br />
									<span className="text-secondary">
										{t('KeyMatrix:pin-label', { pin })}
									</span>
								</div>
								<div className="flex-grow-1">
									<CustomSelect
										inputId={`matrix-switch-${index}`}
										isClearable
										isSearchable
										options={actionOptions}
										value={{
											label: invert(BUTTON_ACTIONS)[action],
											value: action,
										}}
										getOptionLabel={(option) => getActionLabel(option.label)}
										onChange={(change) =>
											setAction(index, change?.value ?? NONE)
										}
									/>
								</div>
							</div>
						);
					})}
				</div>
			</div>
			<FormCheck
				label={t('Common:switch-enabled')}
				type="switch"
				id="KeyMatrixAddonButton"
				reverse
				isInvalid={false}
				checked={Boolean(options.enabled)}
				onChange={() => setOptions({ ...options, enabled: !options.enabled })}
			/>
			<Button type="button" onClick={save} className="mt-3">
				{t('KeyMatrix:save-label')}
			</Button>
			{message && (
				<Alert variant="secondary" className="mt-3">
					{message}
				</Alert>
			)}
		</Section>
	);
};

export default KeyMatrix;
