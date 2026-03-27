//
//  ContentView.swift
//  Spinix
//
//  Created by Cristian Baluta on 22.02.2026.
//

import SwiftUI

struct ContentView: View {
    @EnvironmentObject var model: Model
    var body: some View {
        VStack(spacing: 16) {
            Button("Connect") {
                model.start()
            }
            Text(model.labelSubscription)
            HStack {
                Text("RX:")
                Text(model.rxText)
                Button("GET") {
                    model.get()
                }
            }.textFieldStyle(.roundedBorder)
            HStack {
                Text("TX:")
                TextField("TX", text: $model.txText).textFieldStyle(.roundedBorder)
                Button("POST") {
                    model.post()
                }
            }
            HStack {
                Text("Wheel circumference (m):")
                TextField("0.67", text: $model.wheelCircumferenceText).textFieldStyle(.roundedBorder)
                Button("POST") {
                    model.postWheelCircumference()
                }
            }
            HStack {
                Button("BLUE") {
                    model.led("SET_BLUE")
                }
                Button("RED") {
                    model.led("SET_RED")
                }
                Button("GREEN") {
                    model.led("SET_GREEN")
                }
            }
        }
        .padding()
    }
}

#Preview {
    ContentView()
}
