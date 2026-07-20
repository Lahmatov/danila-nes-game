import XCTest
@testable import FairytaleBook

final class StoreManagerTests: XCTestCase {
    var storeManager: StoreManager!
    var testDefaults: UserDefaults!

    override func setUp() {
        super.setUp()
        testDefaults = UserDefaults(suiteName: "com.lahmatov.fairytalebook.test")!
        testDefaults.removePersistentDomain(forName: "com.lahmatov.fairytalebook.test")
        storeManager = StoreManager(defaults: testDefaults)
    }

    override func tearDown() {
        testDefaults.removePersistentDomain(forName: "com.lahmatov.fairytalebook.test")
        super.tearDown()
    }

    // MARK: - consumeStoryAllowance Tests

    func testConsumeReturnsPlus_WhenPlusIsActive() {
        storeManager.isPlusActive = true
        storeManager.purchasedStoryCredits = 5
        storeManager.remainingFreeStories = 3

        let charge = storeManager.consumeStoryAllowance()

        XCTAssertEqual(charge, .plus)
        XCTAssertEqual(storeManager.purchasedStoryCredits, 5, "Plus should not consume purchased credits")
        XCTAssertEqual(storeManager.remainingFreeStories, 3, "Plus should not consume free stories")
    }

    func testConsumePurchasedCredits_WhenPlusInactiveAndCreditsAvailable() {
        storeManager.isPlusActive = false
        storeManager.purchasedStoryCredits = 3
        storeManager.remainingFreeStories = 2

        let charge = storeManager.consumeStoryAllowance()

        XCTAssertEqual(charge, .purchased)
        XCTAssertEqual(storeManager.purchasedStoryCredits, 2, "Should decrement purchased credits")
        XCTAssertEqual(storeManager.remainingFreeStories, 2, "Free stories should not be touched")
    }

    func testConsumeFreeStories_WhenPlusInactiveAndNoPurchasedCredits() {
        storeManager.isPlusActive = false
        storeManager.purchasedStoryCredits = 0
        storeManager.remainingFreeStories = 1

        let charge = storeManager.consumeStoryAllowance()

        XCTAssertEqual(charge, .free)
        XCTAssertEqual(storeManager.purchasedStoryCredits, 0)
        XCTAssertEqual(storeManager.remainingFreeStories, 0, "Should decrement free stories")
    }

    func testConsumeDenied_WhenNoAllowanceRemains() {
        storeManager.isPlusActive = false
        storeManager.purchasedStoryCredits = 0
        storeManager.remainingFreeStories = 0

        let charge = storeManager.consumeStoryAllowance()

        XCTAssertEqual(charge, .denied)
        XCTAssertEqual(storeManager.purchasedStoryCredits, 0)
        XCTAssertEqual(storeManager.remainingFreeStories, 0)
    }

    func testConsumeMultipleTimes_PrioritizesCorrectly() {
        storeManager.isPlusActive = false
        storeManager.purchasedStoryCredits = 2
        storeManager.remainingFreeStories = 1

        let charge1 = storeManager.consumeStoryAllowance()
        let charge2 = storeManager.consumeStoryAllowance()
        let charge3 = storeManager.consumeStoryAllowance()
        let charge4 = storeManager.consumeStoryAllowance()

        XCTAssertEqual(charge1, .purchased)
        XCTAssertEqual(charge2, .purchased)
        XCTAssertEqual(charge3, .free)
        XCTAssertEqual(charge4, .denied)
    }

    // MARK: - refundStoryAllowance Tests

    func testRefundPurchasedCredit_Increments() {
        storeManager.purchasedStoryCredits = 2

        storeManager.refundStoryAllowance(.purchased)

        XCTAssertEqual(storeManager.purchasedStoryCredits, 3)
    }

    func testRefundFreeStory_Increments() {
        storeManager.remainingFreeStories = 1

        storeManager.refundStoryAllowance(.free)

        XCTAssertEqual(storeManager.remainingFreeStories, 2)
    }

    func testRefundPlus_DoesNothing() {
        storeManager.purchasedStoryCredits = 1
        storeManager.remainingFreeStories = 1

        storeManager.refundStoryAllowance(.plus)

        XCTAssertEqual(storeManager.purchasedStoryCredits, 1)
        XCTAssertEqual(storeManager.remainingFreeStories, 1)
    }

    func testRefundDenied_DoesNothing() {
        storeManager.purchasedStoryCredits = 0
        storeManager.remainingFreeStories = 0

        storeManager.refundStoryAllowance(.denied)

        XCTAssertEqual(storeManager.purchasedStoryCredits, 0)
        XCTAssertEqual(storeManager.remainingFreeStories, 0)
    }

    // MARK: - Persistence Tests

    func testPurchasedCreditsPersistedToUserDefaults() {
        storeManager.purchasedStoryCredits = 5

        let newManager = StoreManager(defaults: testDefaults)

        XCTAssertEqual(newManager.purchasedStoryCredits, 5)
    }

    func testFreeStoriesPersistedToUserDefaults() {
        storeManager.remainingFreeStories = 3

        let newManager = StoreManager(defaults: testDefaults)

        XCTAssertEqual(newManager.remainingFreeStories, 3)
    }

    func testDefaultFreeStoriesIsOne_WhenNotPersisted() {
        testDefaults.removePersistentDomain(forName: "com.lahmatov.fairytalebook.test")

        let newManager = StoreManager(defaults: testDefaults)

        XCTAssertEqual(newManager.remainingFreeStories, 1, "Default should be 1")
    }

    func testConsumeAndPersist_AfterReload() {
        storeManager.purchasedStoryCredits = 3
        storeManager.remainingFreeStories = 2

        // Consume credits
        let charge1 = storeManager.consumeStoryAllowance()
        let charge2 = storeManager.consumeStoryAllowance()

        XCTAssertEqual(charge1, .purchased)
        XCTAssertEqual(charge2, .purchased)

        // Reload and verify persistence
        let newManager = StoreManager(defaults: testDefaults)

        XCTAssertEqual(newManager.purchasedStoryCredits, 1)
        XCTAssertEqual(newManager.remainingFreeStories, 2)
    }

    func testRefundAndPersist_AfterReload() {
        storeManager.purchasedStoryCredits = 1
        storeManager.remainingFreeStories = 0

        storeManager.refundStoryAllowance(.purchased)

        let newManager = StoreManager(defaults: testDefaults)

        XCTAssertEqual(newManager.purchasedStoryCredits, 2)
    }

    // MARK: - Integration Tests

    func testConsumeAndRefund_RestoresState() {
        storeManager.purchasedStoryCredits = 2

        let charge = storeManager.consumeStoryAllowance()
        XCTAssertEqual(storeManager.purchasedStoryCredits, 1)

        storeManager.refundStoryAllowance(charge)
        XCTAssertEqual(storeManager.purchasedStoryCredits, 2)
    }

    func testComplexScenario_MultipleConsumeAndRefund() {
        storeManager.purchasedStoryCredits = 2
        storeManager.remainingFreeStories = 1

        // Consume 3 stories (2 purchased + 1 free)
        let c1 = storeManager.consumeStoryAllowance()
        let c2 = storeManager.consumeStoryAllowance()
        let c3 = storeManager.consumeStoryAllowance()

        XCTAssertEqual(c1, .purchased)
        XCTAssertEqual(c2, .purchased)
        XCTAssertEqual(c3, .free)
        XCTAssertEqual(storeManager.purchasedStoryCredits, 0)
        XCTAssertEqual(storeManager.remainingFreeStories, 0)

        // Refund c2 (purchased)
        storeManager.refundStoryAllowance(c2)
        XCTAssertEqual(storeManager.purchasedStoryCredits, 1)

        // Refund c3 (free)
        storeManager.refundStoryAllowance(c3)
        XCTAssertEqual(storeManager.remainingFreeStories, 1)

        // c1 also consumed, refund it
        storeManager.refundStoryAllowance(c1)
        XCTAssertEqual(storeManager.purchasedStoryCredits, 2)
        XCTAssertEqual(storeManager.remainingFreeStories, 1)
    }

    func testPlusActivationAfterPartialConsumption() {
        storeManager.purchasedStoryCredits = 2
        storeManager.remainingFreeStories = 1
        storeManager.isPlusActive = false

        // Consume 2 purchased
        let c1 = storeManager.consumeStoryAllowance()
        let c2 = storeManager.consumeStoryAllowance()

        XCTAssertEqual(c1, .purchased)
        XCTAssertEqual(c2, .purchased)
        XCTAssertEqual(storeManager.purchasedStoryCredits, 0)

        // Activate plus subscription
        storeManager.isPlusActive = true

        // Next consumption should be plus (not free)
        let c3 = storeManager.consumeStoryAllowance()
        XCTAssertEqual(c3, .plus)
        XCTAssertEqual(storeManager.remainingFreeStories, 1, "Plus should not consume free stories")
    }
}
