//
//  Model.swift
//  Spinix
//
//  Created by Cristian Baluta on 22.02.2026.
//
import BlueSwift
import CoreBluetooth
import SwiftUICore

class Model: NSObject, ObservableObject {

    @Published var labelSubscription: String = ""
    @Published var rxText: String = ""
    @Published var txText: String = ""
    @Published var wheelCircumferenceText: String = ""

    let uuidService =         CBUUID(string: "25AE1441-05D3-4C5B-8281-93D4E07420CF")
    let uuidCharForRead =     CBUUID(string: "25AE1442-05D3-4C5B-8281-93D4E07420CF")// Request data from the peripheral
    let uuidCharForWrite =    CBUUID(string: "25AE1442-05D3-4C5B-8281-93D4E07420CF")// Sends commands to peripheral
    let uuidCharForIndicate = CBUUID(string: "25AE1442-05D3-4C5B-8281-93D4E07420CF")// Receive data from peripheral

    var bleCentral: CBCentralManager!
    var connectedPeripheral: CBPeripheral?

    enum BLELifecycleState: String {
        case bluetoothNotReady
        case disconnected
        case scanning
        case connecting
        case connectedDiscovering
        case connected
    }
    var lifecycleState = BLELifecycleState.bluetoothNotReady {
        didSet {
            print("state = \(lifecycleState)")
            labelSubscription = lifecycleState.rawValue
        }
    }

//    func start() {
//        let connection = BluetoothConnection.shared
//        let characteristic = try! Characteristic(uuid: "25AE1442-05D3-4C5B-8281-93D4E07420CF", shouldObserveNotification: true)
//        let service = try! Service(uuid: "25AE1441-05D3-4C5B-8281-93D4E07420CF", characteristics: [characteristic])
//        let configuration = try! Configuration(services: [service], advertisement: "25AE1444-05D3-4C5B-8281-93D4E07420CF")
//        let peripheral = Peripheral(configuration: configuration)
//        connection.connect(peripheral) { error in
//            // do awesome stuff
//            print("Connected with \(error)")
//        }
//    }

    override init() {
        super.init()
        bleCentral = CBCentralManager(delegate: self, queue: DispatchQueue.main)
    }

    func start() {
        bleRestartLifecycle()
    }

    func bleRestartLifecycle() {
        guard bleCentral.state == .poweredOn else {
            connectedPeripheral = nil
            lifecycleState = .bluetoothNotReady
            return
        }
        if let oldPeripheral = connectedPeripheral {
            bleCentral.cancelPeripheralConnection(oldPeripheral)
        }
        connectedPeripheral = nil
        bleScan()
    }

    func bleScan() {
        lifecycleState = .scanning
        bleCentral.scanForPeripherals(withServices: [uuidService], options: nil)
//        bleCentral.scanForPeripherals(withServices: nil, options: nil)
    }

    func bleConnect(to peripheral: CBPeripheral) {
        connectedPeripheral = peripheral
        lifecycleState = .connecting
        bleCentral.connect(peripheral, options: nil)
    }

    func bleDisconnect() {
        if bleCentral.isScanning {
            bleCentral.stopScan()
        }
        if let peripheral = connectedPeripheral {
            bleCentral.cancelPeripheralConnection(peripheral)
        }
        lifecycleState = .disconnected
    }

    func bleReadCharacteristic(uuid: CBUUID) {
        guard let characteristic = getCharacteristic(uuid: uuid) else {
            print("ERROR: read failed, characteristic unavailable, uuid = \(uuid.uuidString)")
            return
        }
        connectedPeripheral?.readValue(for: characteristic)
    }

    func bleWriteCharacteristic(uuid: CBUUID, data: Data) {
        guard let characteristic = getCharacteristic(uuid: uuid) else {
            print("ERROR: write failed, characteristic unavailable, uuid = \(uuid.uuidString)")
            return
        }
        connectedPeripheral?.writeValue(data, for: characteristic, type: .withResponse)
    }

    func getCharacteristic(uuid: CBUUID) -> CBCharacteristic? {
        guard let service = connectedPeripheral?.services?.first(where: { $0.uuid == uuidService }) else {
            return nil
        }
        return service.characteristics?.first { $0.uuid == uuid }
    }

    private func bleGetStatusString() -> String {
        guard let bleCentral = bleCentral else { return "not initialized" }
        switch bleCentral.state {
        case .unauthorized:
            return bleCentral.state.stringValue + " (allow in Settings)"
        case .poweredOff:
            return "Bluetooth OFF"
        case .poweredOn:
            return "ON, \(lifecycleState)"
        default:
            return bleCentral.state.stringValue
        }
    }

    func get() {
        bleReadCharacteristic(uuid: uuidCharForRead)
    }

    func post() {
        rxText = "GET_GPS command sent"
        let data = "GET_GPS".data(using: .utf8) ?? Data()
        bleWriteCharacteristic(uuid: uuidCharForWrite, data: data)
    }

    func postWheelCircumference() {
        let command = String(format: "SET_WHEEL:%.3f", Double(wheelCircumferenceText) ?? 0.67)
        let data = command.data(using: .utf8) ?? Data()
        bleWriteCharacteristic(uuid: uuidCharForWrite, data: data)
    }

    func led(_ command: String) {
        let data = command.data(using: .utf8) ?? Data()
        bleWriteCharacteristic(uuid: uuidCharForWrite, data: data)
    }

    func postlongText() {
        let longText = "Information in this document, including URL references, is subject to change without notice. THIS DOCUMENT IS PROVIDED AS IS WITH NO WARRANTIES WHATSOEVER, INCLUDING ANY WARRANTY OF MERCHANTABILITY, NON-INFRINGEMENT, FITNESS FOR ANY PARTICULAR PURPOSE, OR ANY WARRANTY OTHERWISE ARISING OUT OF ANY PROPOSAL, SPECIFICATION OR SAMPLE. All liability, including liability for infringement of any proprietary rights, relating to use of information in this document is disclaimed. No licenses express or implied, by estoppel or otherwise, to any intellectual property rights are granted herein. The Wi-Fi Alliance Member logo is a trademark of the Wi-Fi Alliance. The Bluetooth logo is a registered trademark of Bluetooth SIG. All trade names, trademarks and registered trademarks mentioned in this document are property of their respective owners, and are hereby acknowledged. Copyright © 2018 Espressif Inc. All rights reserved."
        let chunks = longText.splitByByteLimit(256-3)

        for chunk in chunks {
            print("writing: \(chunk)")
            let data = chunk.data(using: .utf8) ?? Data()
            bleWriteCharacteristic(uuid: uuidCharForWrite, data: data)
        }
    }
}

extension Model: CBCentralManagerDelegate {
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        print("central didUpdateState: \(central.state.stringValue)")
        bleRestartLifecycle()
    }

    func centralManager(_ central: CBCentralManager,
                        didDiscover peripheral: CBPeripheral,
                        advertisementData: [String: Any],
                        rssi RSSI: NSNumber) {
        print(">>>>> didDiscover {name: \(peripheral.name), rssi: \(peripheral.identifier), state: \(peripheral.state.rawValue), services: \(peripheral.state) advertisementData: \(advertisementData)")

        guard connectedPeripheral == nil else {
            print("didDiscover ignored (connectedPeripheral already set)")
            return
        }

//        if peripheral.name == "ESP" {
            bleCentral.stopScan()
            bleConnect(to: peripheral)
//        }
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        print("didConnect")

        lifecycleState = .connectedDiscovering
        peripheral.delegate = self
        peripheral.discoverServices([uuidService])
//        peripheral.discoverServices(nil)
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        if peripheral === connectedPeripheral {
            print("didFailToConnect")
            connectedPeripheral = nil
            bleRestartLifecycle()
        } else {
            print("didFailToConnect, unknown peripheral, ingoring")
        }
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        if peripheral === connectedPeripheral {
            print("didDisconnect")
            connectedPeripheral = nil
            bleRestartLifecycle()
        } else {
            print("didDisconnect, unknown peripheral, ingoring")
        }
    }
}

extension Model: CBPeripheralDelegate {
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        print(">>>>>>>>didDiscoverServices: \(peripheral.services)")
        guard let service = peripheral.services?.first(where: { $0.uuid == uuidService }) else {
            print("ERROR: didDiscoverServices, service NOT found\nerror = \(String(describing: error)), disconnecting")
            bleCentral.cancelPeripheralConnection(peripheral)
            return
        }

        print("didDiscoverServices, service found")
        peripheral.discoverCharacteristics([uuidCharForRead, uuidCharForWrite, uuidCharForIndicate], for: service)
    }

    func peripheral(_ peripheral: CBPeripheral, didModifyServices invalidatedServices: [CBService]) {
        print("didModifyServices")
        // usually this method is called when Android application is terminated
        if invalidatedServices.first(where: { $0.uuid == uuidService }) != nil {
            print("disconnecting because peripheral removed the required service")
            bleCentral.cancelPeripheralConnection(peripheral)
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        print("didDiscoverCharacteristics \(error == nil ? "OK" : "error: \(String(describing: error))")")
        print(">>>>>> didDiscoverCharacteristics: \(service.characteristics ?? [])")
        if let charIndicate = service.characteristics?.first(where: { $0.uuid == uuidCharForIndicate }) {
            peripheral.setNotifyValue(true, for: charIndicate)
        } else {
            print("WARN: characteristic for indication not found")
            lifecycleState = .connected
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        guard error == nil else {
            print("didUpdateValue error: \(String(describing: error))")
            return
        }
        let data = characteristic.value ?? Data()
        let stringValue = String(data: data, encoding: .utf8) ?? ""
        if characteristic.uuid == uuidCharForRead {
            rxText += stringValue
        } else if characteristic.uuid == uuidCharForIndicate {
//            textFieldDataForIndicate.text = stringValue
        }
        print("didUpdateValue '\(stringValue)'")
    }

    func peripheral(_ peripheral: CBPeripheral,
                    didWriteValueFor characteristic: CBCharacteristic,
                    error: Error?) {
        print("didWrite \(error == nil ? "OK" : "error: \(String(describing: error))")")
    }

    func peripheral(_ peripheral: CBPeripheral,
                    didUpdateNotificationStateFor characteristic: CBCharacteristic,
                    error: Error?) {
        guard error == nil else {
            print("didUpdateNotificationState error\n\(String(describing: error))")
            lifecycleState = .connected
            return
        }

        if characteristic.uuid == uuidCharForIndicate {
            let info = characteristic.isNotifying ? "Subscribed" : "Not subscribed"
//            labelSubscription.text = info
            print(info)
        }
        lifecycleState = .connected
    }
}

extension CBManagerState {
    var stringValue: String {
        switch self {
        case .unknown: return "unknown"
        case .resetting: return "resetting"
        case .unsupported: return "unsupported"
        case .unauthorized: return "unauthorized"
        case .poweredOff: return "poweredOff"
        case .poweredOn: return "poweredOn"
        @unknown default: return "\(rawValue)"
        }
    }
}

extension String {
    func split(by length: Int) -> [String] {
        var startIndex = self.startIndex
        var results = [Substring]()

        while startIndex < self.endIndex {
            let endIndex = self.index(startIndex, offsetBy: length, limitedBy: self.endIndex) ?? self.endIndex
            results.append(self[startIndex..<endIndex])
            startIndex = endIndex
        }

        return results.map { String($0) }
    }
}

extension String {
    func splitByByteLimit(_ limit: Int, encoding: String.Encoding = .utf8) -> [String] {
        var chunks: [String] = []
        var currentChunk = ""
        var currentBytes = 0

        for character in self {
            let charBytes = String(character).lengthOfBytes(using: encoding)

            if currentBytes + charBytes > limit {
                chunks.append(currentChunk)
                currentChunk = String(character)
                currentBytes = charBytes
            } else {
                currentChunk.append(character)
                currentBytes += charBytes
            }
        }

        if !currentChunk.isEmpty {
            chunks.append(currentChunk)
        }

        return chunks
    }
}
