//
//  SpinixApp.swift
//  Spinix
//
//  Created by Cristian Baluta on 22.02.2026.
//

import SwiftUI

@main
struct SpinixApp: App {
    let model = Model()
    var body: some Scene {
        WindowGroup {
            ContentView()
                .onAppear() {
//                    model.start()
                }
                .environmentObject(model)
        }
    }
}
