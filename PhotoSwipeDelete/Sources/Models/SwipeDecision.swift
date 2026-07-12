import Foundation

enum SwipeDecision: Equatable {
    case keep
    case delete
}

struct SwipeRecord: Equatable {
    let assetID: String
    let decision: SwipeDecision
}
