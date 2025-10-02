class HydroponicController {
    constructor() {
        this.updateInterval = null;
        this.init();
    }

    init() {
        this.setupEventListeners();
        this.loadInitialData();
        this.startAutoUpdate();
    }

    setupEventListeners() {
        // Form submissions
        document.getElementById('lights-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.updateLights();
        });

        document.getElementById('pump-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.updatePump();
        });

        document.getElementById('heater-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.updateHeater();
        });

        document.getElementById('fan-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.updateFan();
        });

        // Configuration buttons
        document.getElementById('save-config').addEventListener('click', () => {
            this.saveConfig();
        });

        document.getElementById('load-config').addEventListener('click', () => {
            this.loadConfig();
        });

        // Pump mode change
        document.getElementById('pump-mode').addEventListener('change', (e) => {
            this.toggleHumidityControls(e.target.value === 'humidity');
        });
    }

    async loadInitialData() {
        try {
            const [status, config] = await Promise.all([
                this.fetchData('/api/status'),
                this.fetchData('/api/config')
            ]);
            
            this.updateUI(status, config);
        } catch (error) {
            console.error('Failed to load initial data:', error);
            this.showToast('Failed to load initial data', 'error');
        }
    }

    startAutoUpdate() {
        this.updateInterval = setInterval(() => {
            this.updateStatus();
        }, 2000);
    }

    async updateStatus() {
        try {
            const status = await this.fetchData('/api/status');
            this.updateUI(status);
        } catch (error) {
            console.error('Failed to update status:', error);
        }
    }

    async fetchData(url) {
        const response = await fetch(url);
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        return await response.json();
    }

    async postData(url, data) {
        const response = await fetch(url, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(data)
        });
        
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        
        return await response.json();
    }

    updateUI(status, config = null) {
        // Update sensor readings
        this.updateSensorReading('temperature-value', status.temperature, '°C');
        this.updateSensorReading('humidity-value', status.humidity, '%');

        // Update device status
        this.updateDeviceStatus('lights-status', status.lights_on);
        this.updateDeviceStatus('pump-status', status.pump_on);
        this.updateDeviceStatus('heater-status', status.heater_on);
        this.updateDeviceStatus('fan-status', status.fan_on);

        // Update WiFi and time status
        document.getElementById('wifi-status').textContent = status.wifi_connected ? 'Connected' : 'Disconnected';
        document.getElementById('time-status').textContent = status.time_synced ? 'Synced' : 'Syncing...';

        // Update form values if config is provided
        if (config) {
            document.getElementById('lights-start').value = this.secondsToTimeString(config.lights_start_s);
            document.getElementById('lights-end').value = this.secondsToTimeString(config.lights_end_s);
            document.getElementById('pump-on').value = config.pump_on_sec;
            document.getElementById('pump-period').value = config.pump_period;
            document.getElementById('pump-mode').value = config.humidity_mode ? 'humidity' : 'timer';
            document.getElementById('heater-setpoint').value = config.heater_setpoint_c;
            document.getElementById('humidity-threshold').value = config.humidity_threshold;
            
            this.toggleHumidityControls(config.humidity_mode);
        }
    }

    updateSensorReading(elementId, value, unit) {
        const element = document.getElementById(elementId);
        if (value === -999 || value === null || value === undefined) {
            element.textContent = `--${unit}`;
        } else {
            element.textContent = `${value.toFixed(1)}${unit}`;
        }
    }

    updateDeviceStatus(elementId, isOn) {
        const element = document.getElementById(elementId);
        element.textContent = isOn ? 'ON' : 'OFF';
        element.className = `status-indicator ${isOn ? 'on' : 'off'}`;
    }

    toggleHumidityControls(show) {
        const humidityControls = document.getElementById('humidity-controls');
        humidityControls.style.display = show ? 'block' : 'none';
    }

    async updateLights() {
        try {
            const startTime = document.getElementById('lights-start').value;
            const endTime = document.getElementById('lights-end').value;
            
            await this.postData('/api/lights', {
                start_time: startTime,
                end_time: endTime
            });
            
            this.showToast('Lights schedule updated successfully', 'success');
        } catch (error) {
            console.error('Failed to update lights:', error);
            this.showToast('Failed to update lights schedule', 'error');
        }
    }

    async updatePump() {
        try {
            const mode = document.getElementById('pump-mode').value;
            const onSec = parseInt(document.getElementById('pump-on').value);
            const period = parseInt(document.getElementById('pump-period').value);
            const humidityThreshold = parseFloat(document.getElementById('humidity-threshold').value);
            
            await this.postData('/api/pump', {
                mode: mode,
                on_sec: onSec,
                period: period,
                humidity_threshold: humidityThreshold
            });
            
            this.showToast('Pump settings updated successfully', 'success');
        } catch (error) {
            console.error('Failed to update pump:', error);
            this.showToast('Failed to update pump settings', 'error');
        }
    }

    async updateHeater() {
        try {
            const setpoint = parseFloat(document.getElementById('heater-setpoint').value);
            
            await this.postData('/api/heater', {
                setpoint: setpoint
            });
            
            this.showToast('Heater setpoint updated successfully', 'success');
        } catch (error) {
            console.error('Failed to update heater:', error);
            this.showToast('Failed to update heater setpoint', 'error');
        }
    }

    async updateFan() {
        try {
            const state = document.getElementById('fan-state').value;
            
            await this.postData('/api/fan', {
                state: state
            });
            
            this.showToast('Fan state updated successfully', 'success');
        } catch (error) {
            console.error('Failed to update fan:', error);
            this.showToast('Failed to update fan state', 'error');
        }
    }

    async saveConfig() {
        try {
            await this.postData('/api/save', {});
            this.showToast('Configuration saved successfully', 'success');
        } catch (error) {
            console.error('Failed to save config:', error);
            this.showToast('Failed to save configuration', 'error');
        }
    }

    async loadConfig() {
        try {
            const config = await this.fetchData('/api/config');
            this.updateUI(null, config);
            this.showToast('Configuration loaded successfully', 'success');
        } catch (error) {
            console.error('Failed to load config:', error);
            this.showToast('Failed to load configuration', 'error');
        }
    }

    showToast(message, type = 'info') {
        const toastContainer = document.getElementById('toast-container');
        const toast = document.createElement('div');
        toast.className = `toast ${type}`;
        toast.textContent = message;
        
        toastContainer.appendChild(toast);
        
        setTimeout(() => {
            toast.remove();
        }, 3000);
    }

    secondsToTimeString(seconds) {
        const hours = Math.floor(seconds / 3600);
        const minutes = Math.floor((seconds % 3600) / 60);
        return `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}`;
    }
}

document.addEventListener('DOMContentLoaded', () => {
    new HydroponicController();
});
