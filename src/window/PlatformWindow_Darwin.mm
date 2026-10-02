/*
 * Brisk
 *
 * Cross-platform application framework
 * --------------------------------------------------------------
 *
 * Copyright (C) 2025 Brisk Developers
 *
 * This file is part of the Brisk library.
 *
 * Brisk is dual-licensed under the GNU General Public License, version 2 (GPL-2.0+),
 * and a commercial license. You may use, modify, and distribute this software under
 * the terms of the GPL-2.0+ license if you comply with its conditions.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * If you do not wish to be bound by the GPL-2.0+ license, you must purchase a commercial
 * license. For commercial licensing options, please visit: https://brisklib.com
 */
#define BRISK_ALLOW_OS_HEADERS 1
#include <algorithm>

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

#include <brisk/core/App.hpp>
#include <brisk/core/BasicTypes.hpp>
#include <brisk/core/Localization.hpp>
#include <brisk/core/Time.hpp>
#include <brisk/graphics/NativeWindowHandle.hpp>
#include <brisk/graphics/internal/NSTypes.hpp>
#include <brisk/window/Display.hpp>
#include <brisk/window/Types.hpp>
#include <brisk/window/Window.hpp>

#include "PlatformWindow.hpp"

// Set up the menu bar (manually)
// This is nasty, nasty stuff -- calls to undocumented semi-private APIs that
// could go away at any moment, lots of stuff that really should be
// localize(d|able), etc.  Add a nib to save us this horror.
//
static void createMenuBar(void) {
    NSString* appName = Brisk::toNSString(Brisk::appMetadata.name);

    NSMenu* bar       = [[NSMenu alloc] init];
    [NSApp setMainMenu:bar];

    NSMenuItem* appMenuItem = [bar addItemWithTitle:@"" action:NULL keyEquivalent:@""];
    NSMenu* appMenu         = [[NSMenu alloc] init];
    [appMenuItem setSubmenu:appMenu];

    [appMenu addItemWithTitle:[NSString stringWithFormat:@"About %@", appName]
                       action:@selector(orderFrontStandardAboutPanel:)
                keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    NSMenu* servicesMenu = [[NSMenu alloc] init];
    [NSApp setServicesMenu:servicesMenu];
    [[appMenu addItemWithTitle:@"Services" action:NULL keyEquivalent:@""] setSubmenu:servicesMenu];

    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:[NSString stringWithFormat:@"Hide %@", appName]
                       action:@selector(hide:)
                keyEquivalent:@"h"];
    [[appMenu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"]
        setKeyEquivalentModifierMask:NSEventModifierFlagOption | NSEventModifierFlagCommand];
    [appMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:[NSString stringWithFormat:@"Quit %@", appName]
                       action:@selector(terminate:)
                keyEquivalent:@"q"];

    NSMenuItem* windowMenuItem = [bar addItemWithTitle:@"" action:NULL keyEquivalent:@""];

    NSMenu* windowMenu         = [[NSMenu alloc] initWithTitle:@"Window"];
    [NSApp setWindowsMenu:windowMenu];
    [windowMenuItem setSubmenu:windowMenu];

    [windowMenu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
    [windowMenu addItemWithTitle:@"Zoom" action:@selector(performZoom:) keyEquivalent:@""];
    [windowMenu addItem:[NSMenuItem separatorItem]];
    [windowMenu addItemWithTitle:@"Bring All to Front" action:@selector(arrangeInFront:) keyEquivalent:@""];

    // TODO: Make this appear at the bottom of the menu (for consistency)
    [windowMenu addItem:[NSMenuItem separatorItem]];
    [[windowMenu addItemWithTitle:@"Enter Full Screen" action:@selector(toggleFullScreen:) keyEquivalent:@"f"]
        setKeyEquivalentModifierMask:NSEventModifierFlagControl | NSEventModifierFlagCommand];
}

@interface AppHelper : NSObject
@end

@implementation AppHelper

- (void)doNothing:(id)object {
}

@end // AppHelper

@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AppDelegate

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
    Brisk::windowApplication->quit();

    return NSTerminateCancel;
}

- (void)applicationDidChangeScreenParameters:(NSNotification*)notification {
    Brisk::Internal::updateDisplays();
}

- (void)applicationWillFinishLaunching:(NSNotification*)notification {
    // Menu bar setup must go between sharedApplication and finishLaunching
    // in order to properly emulate the behavior of NSApplicationMain

    createMenuBar();
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    Brisk::PlatformWindow::postEmptyEvent();
    [NSApp stop:nil];
}

@end

namespace Brisk {

struct PlatformWindowData {
    __strong id parent   = nil;
    NSWindow* window     = nil;
    NSView* view         = nil;
    __strong id delegate = nil;
    float scale{ -1 };
    bool occluded = false;
    NSPoint cascadePoint{ 0, 0 };
};

namespace Internal {

struct SystemCursor {
    NSCursor* cursor = nil;

    ~SystemCursor() {}
};

static NSCursor* createCursor(const ImageAccess<ImageFormat::RGBA, AccessMode::R>& image, int xhot, int yhot,
                              float scale) {
    @autoreleasepool {
        NSBitmapImageRep* rep =
            [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                    pixelsWide:image.width()
                                                    pixelsHigh:image.height()
                                                 bitsPerSample:8
                                               samplesPerPixel:4
                                                      hasAlpha:YES
                                                      isPlanar:NO
                                                colorSpaceName:NSCalibratedRGBColorSpace
                                                  bitmapFormat:NSBitmapFormatAlphaNonpremultiplied
                                                   bytesPerRow:image.width() * 4
                                                  bitsPerPixel:32];

        if (rep == nil)
            return nullptr;
        image.writeTo(BytesMutableView(reinterpret_cast<std::byte*>([rep bitmapData]), image.byteSize()));
        NSSize dipSize = NSMakeSize(std::round(image.width() / scale), std::round(image.height() / scale));
        NSPoint dipHot = NSMakePoint(std::round(xhot / scale), std::round(yhot / scale));

        NSImage* native =
            [[NSImage alloc] initWithSize:NSMakeSize(image.width() / scale, image.height() / scale)];
        [native addRepresentation:rep];

        [native setSize:dipSize];
        [[[native representations] objectAtIndex:0] setSize:dipSize];

        return [[NSCursor alloc] initWithImage:native hotSpot:dipHot];
    } // autoreleasepool
}

Rc<SystemCursor> PlatformCursors::cursorFromImage(const Rc<Image>& image, Point point, float scale) {
    return rcnew SystemCursor{ createCursor(image->mapRead<ImageFormat::RGBA>(), point.x, point.y, scale) };
}

Rc<SystemCursor> PlatformCursors::getSystemCursor(Cursor shape) {
    Rc<SystemCursor> result = rcnew SystemCursor{};
    @autoreleasepool {

        SEL cursorSelector = NULL;

        // HACK: Try to use a private message
        switch (shape) {
        case Cursor::EWResize:
            cursorSelector = NSSelectorFromString(@"_windowResizeEastWestCursor");
            break;
        case Cursor::NSResize:
            cursorSelector = NSSelectorFromString(@"_windowResizeNorthSouthCursor");
            break;
        case Cursor::NWSEResize:
            cursorSelector = NSSelectorFromString(@"_windowResizeNorthWestSouthEastCursor");
            break;
        case Cursor::NESWResize:
            cursorSelector = NSSelectorFromString(@"_windowResizeNorthEastSouthWestCursor");
            break;
        default:
            break;
        }

        if (cursorSelector && [NSCursor respondsToSelector:cursorSelector]) {
            id object = [NSCursor performSelector:cursorSelector];
            if ([object isKindOfClass:[NSCursor class]])
                result->cursor = object;
        }

        if (!result->cursor) {
            switch (shape) {
            case Cursor::Arrow:
                result->cursor = [NSCursor arrowCursor];
                break;
            case Cursor::IBeam:
                result->cursor = [NSCursor IBeamCursor];
                break;
            case Cursor::Crosshair:
                result->cursor = [NSCursor crosshairCursor];
                break;
            case Cursor::Hand:
                result->cursor = [NSCursor pointingHandCursor];
                break;
            case Cursor::EWResize:
            case Cursor::HResize:
                result->cursor = [NSCursor resizeLeftRightCursor];
                break;
            case Cursor::NSResize:
            case Cursor::VResize:
                result->cursor = [NSCursor resizeUpDownCursor];
                break;
            case Cursor::AllResize:
                result->cursor = [NSCursor closedHandCursor];
                break;
            case Cursor::NotAllowed:
                result->cursor = [NSCursor operationNotAllowedCursor];
                break;
            default:
                break;
            }
        }

        if (!result->cursor) {
            BRISK_SOFT_ASSERT_MSG("Cocoa: Standard cursor shape unavailable", false);
            return nullptr;
        }
        return result;
    } // autoreleasepool
}

} // namespace Internal

static KeyModifiers getKeyMods(NSUInteger flags) {
    KeyModifiers mods = KeyModifiers::None;

    if (flags & NSEventModifierFlagShift)
        mods |= KeyModifiers::Shift;
    if (flags & NSEventModifierFlagControl)
        mods |= KeyModifiers::Control;
    if (flags & NSEventModifierFlagOption)
        mods |= KeyModifiers::MacosOption;
    if (flags & NSEventModifierFlagCommand)
        mods |= KeyModifiers::MacosCommand;
    if (flags & NSEventModifierFlagCapsLock)
        mods |= KeyModifiers::CapsLock;

    return mods;
}

static const NSRange kEmptyRange = { NSNotFound, 0 };

static struct {
    __strong id helper;
    __strong id delegate;
    __strong id keyUpMonitor;
} staticData;

/*static*/ void PlatformWindow::initialize() {

    staticData.helper = [AppHelper.alloc init];

    [NSThread detachNewThreadSelector:@selector(doNothing:) toTarget:staticData.helper withObject:nil];

    [NSApplication sharedApplication];

    staticData.delegate = [AppDelegate.alloc init];

    [NSApp setDelegate:staticData.delegate];

    NSEvent* (^block)(NSEvent*) = ^NSEvent*(NSEvent* event) {
      if ([event modifierFlags] & NSEventModifierFlagCommand)
          [[NSApp keyWindow] sendEvent:event];

      return event;
    };

    staticData.keyUpMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyUp handler:block];

    // Press and Hold prevents some keys from emitting repeated characters
    NSDictionary* defaults  = @{@"ApplePressAndHoldEnabled" : @NO};
    [[NSUserDefaults standardUserDefaults] registerDefaults:defaults];

    Internal::updateDisplays();

    if (![[NSRunningApplication currentApplication] isFinishedLaunching])
        [NSApp run];

    // In case we are unbundled, make us a proper UI application
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
}

/*static*/ void PlatformWindow::finalize() {

    [NSApp setDelegate:nil];

    if (staticData.keyUpMonitor)
        [NSEvent removeMonitor:staticData.keyUpMonitor];

    staticData = {}; // Release all objc objects
}

/* static */ void PlatformWindow::pollEvents() {
    @autoreleasepool {
        for (;;) {
            NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                untilDate:[NSDate distantPast]
                                                   inMode:NSDefaultRunLoopMode
                                                  dequeue:YES];
            if (event == nil)
                break;

            [NSApp sendEvent:event];
        }
    } // autoreleasepool
}

/* static */ void PlatformWindow::waitEvents() {
    @autoreleasepool {
        // I wanted to pass NO to dequeue:, and rely on PollEvents to
        // dequeue and send.  For reasons not at all clear to me, passing
        // NO to dequeue: causes this method never to return.
        NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                            untilDate:[NSDate distantFuture]
                                               inMode:NSDefaultRunLoopMode
                                              dequeue:YES];
        [NSApp sendEvent:event];

        pollEvents();
    } // autoreleasepool
}

/* static */ void PlatformWindow::postEmptyEvent() {
    @autoreleasepool {
        NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                            location:NSMakePoint(0, 0)
                                       modifierFlags:0
                                           timestamp:0
                                        windowNumber:0
                                             context:nil
                                             subtype:0
                                               data1:0
                                               data2:0];
        [NSApp postEvent:event atStart:YES];
    } // autoreleasepool
}

/* static */ DblClickParams PlatformWindow::dblClickParams() {
    return {
        NSEvent.doubleClickInterval,
        2,
    };
}

void PlatformWindow::updateVisibility() {
    @autoreleasepool {
        if (m_data->window == nil) {
            // Child window
            [m_data->view setHidden:(m_window->m_visible ? YES : NO)];
        } else {
            bool visible = m_window->m_visible;
            if (visible) {
                [m_data->window orderFront:nil];
                focus();
            } else {
                [m_data->window orderOut:nil];
            }
        }
    }
}

void PlatformWindow::iconify() {
    @autoreleasepool {
        if (m_data->window == nil) {
            // do nothing for child windows
        } else {
            [m_data->window miniaturize:nil];
        }
    } // autoreleasepool
}

void PlatformWindow::restore() {
    @autoreleasepool {
        if (m_data->window == nil) {
            // do nothing for child windows
        } else {
            if ([m_data->window isMiniaturized])
                [m_data->window deminiaturize:nil];
            else if ([m_data->window isZoomed])
                [m_data->window zoom:nil];
        }
    } // autoreleasepool
}

void PlatformWindow::maximize() {
    @autoreleasepool {
        if (m_data->window == nil) {
            // do nothing for child windows
        } else {
            if (![m_data->window isZoomed])
                [m_data->window zoom:nil];
        }
    } // autoreleasepool
}

bool PlatformWindow::isFocused() const {
    @autoreleasepool {
        if (m_data->window == nil)
            return [[m_data->view window] isKeyWindow];
        else
            return [m_data->window isKeyWindow];
    } // autoreleasepool
}

bool PlatformWindow::isIconified() const {
    @autoreleasepool {
        if (m_data->window == nil)
            return false;
        else
            return [m_data->window isMiniaturized];
    } // autoreleasepool
}

bool PlatformWindow::isMaximized() const {
    @autoreleasepool {
        if (m_data->window == nil)
            return false;
        else
            return [m_data->window isZoomed];
    } // autoreleasepool
}

bool PlatformWindow::isVisible() const {
    @autoreleasepool {
        if (m_data->window == nil)
            return [m_data->view isHidden] == NO;
        else
            return [m_data->window isVisible];
    } // autoreleasepool
}

void PlatformWindow::focus() {
    @autoreleasepool {
        // Make us the active application
        // HACK: This is here to prevent applications using only hidden windows from
        //       being activated, but should probably not be done every time any
        //       window is shown
        [NSApp activateIgnoringOtherApps:YES];
        if (m_data->window == nil) {
            [[m_data->view window] makeKeyAndOrderFront:nil];
        } else {
            [m_data->window makeKeyAndOrderFront:nil];
        }
    } // autoreleasepool
}

void PlatformWindow::setOwner(Rc<Window> window) {
    BRISK_LOG_WARN("macOS doesn't implement owner windows");
}

void PlatformWindow::setTitle(std::string_view title) {
    if (m_data->window == nil)
        return;
    [m_data->window setTitle:toNSString(title)];
}

Bytes PlatformWindow::placement() const {
    return {};
}

void PlatformWindow::setPlacement(BytesView data) {
    // TODO: macOS window placement
}

NativeWindowHandle PlatformWindow::getHandle() const {
    if (m_data->window)
        return NativeWindowHandle(m_data->window);
    else
        return NativeWindowHandle(m_data->view);
}

PlatformWindow::~PlatformWindow() {
    mustBeMainThread();

    if (m_data->window != nil) {
        [m_data->window orderOut:nil];
        [m_data->window setDelegate:nil];
    }
    m_data->delegate = nil;

    m_data->view     = nil;
    m_data->window   = nil;
    m_data->parent   = nil;

    // HACK: Allow Cocoa to catch up before returning
    pollEvents();
}

PlatformWindow::PlatformWindow(Window* window, Size windowSize, Point position, WindowStyle style,
                               NativeWindowHandle parent)
    : m_data(new PlatformWindowData{}), m_window(window), m_windowStyle(style), m_windowSize(windowSize),
      m_position(position) {
    mustBeMainThread();
    m_data->parent = (__bridge id)parent.ptr;
    BRISK_ASSERT(m_window);

    bool created = createWindow();
    BRISK_SOFT_ASSERT(created);
    if (!created)
        return;

    updateSize();
    contentScaleChanged(m_scale, m_scale);
}

} // namespace Brisk

using namespace Brisk;

@interface BriskWindowDelegate : NSObject {
    PlatformWindow* window;
}

- (instancetype)initWithWindow:(PlatformWindow*)initWindow;

@end

@implementation BriskWindowDelegate

- (instancetype)initWithWindow:(PlatformWindow*)initWindow {
    self = [super init];
    if (self != nil)
        window = initWindow;

    return self;
}

- (BOOL)windowShouldClose:(id)sender {
    window->closeAttempt();
    return NO;
}

- (void)windowDidResize:(NSNotification*)notification {
    const int maximized = [window->m_data->window isZoomed];
    if (window->m_maximized != maximized) {
        window->windowStateChanged(window->m_iconified, maximized);

        window->m_maximized = maximized;
        // TODO: notify that window became maximized
    }

    const NSRect contentRect = [window->m_data->view frame];
    const NSRect fbRect      = [window->m_data->view convertRectToBacking:contentRect];

    bool sizeChanged         = false;
    if (Brisk::Size(fromNSSize(fbRect.size)) != window->m_framebufferSize) {
        window->m_framebufferSize = fromNSSize(fbRect.size);
        sizeChanged               = true;
    }

    if (Brisk::Size(fromNSSize(contentRect.size)) != window->m_windowSize) {
        window->m_windowSize = fromNSSize(contentRect.size);
        sizeChanged          = true;
    }
    if (sizeChanged) {
        window->windowResized(window->m_windowSize, window->m_framebufferSize);
    }
}

- (void)windowDidMove:(NSNotification*)notification {
    const NSRect contentRect = [window->m_data->view frame];
    if (Brisk::Point(fromNSPoint(contentRect.origin)) != window->m_position) {
        window->m_position = fromNSPoint(contentRect.origin);
        window->windowMoved(window->m_position);
    }
}

- (void)windowDidMiniaturize:(NSNotification*)notification {
    window->windowStateChanged(true, window->m_maximized);
    window->m_iconified = true;
}

- (void)windowDidDeminiaturize:(NSNotification*)notification {
    window->windowStateChanged(false, window->m_maximized);
    window->m_iconified = false;
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
    window->focusChange(true);
}

- (void)windowDidResignKey:(NSNotification*)notification {
    window->focusChange(false);
}

- (void)windowDidChangeOcclusionState:(NSNotification*)notification {
    if ([window->m_data->window occlusionState] & NSWindowOcclusionStateVisible)
        window->m_data->occluded = false;
    else
        window->m_data->occluded = true;
}

@end

@interface BriskView : NSView <NSTextInputClient> {
    PlatformWindow* window;
    NSTrackingArea* trackingArea;
    NSMutableAttributedString* markedText;
}

- (instancetype)initWithWindow:(PlatformWindow*)initWindow;

@end

@implementation BriskView

- (instancetype)initWithWindow:(PlatformWindow*)initWindow {
    self = [super init];
    if (self != nil) {
        window       = initWindow;
        trackingArea = nil;
        markedText   = [[NSMutableAttributedString alloc] init];

        [self updateTrackingAreas];
        [self registerForDraggedTypes:@[ NSPasteboardTypeURL ]];
    }

    return self;
}

- (void)dealloc {
    trackingArea = nil;
    markedText   = nil;
}

- (BOOL)isOpaque {
    if (window->m_data->window == nil)
        return NO;
    return [window->m_data->window isOpaque];
}

- (BOOL)canBecomeKeyView {
    return YES;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (BOOL)wantsUpdateLayer {
    return YES;
}

- (void)updateLayer {
    window->requestRedraw();
}

- (void)cursorUpdate:(NSEvent*)event {
    window->updateCursorImage();
}

- (BOOL)acceptsFirstMouse:(NSEvent*)event {
    return YES;
}

- (PointF)eventToPos:(NSEvent*)event {
    const NSRect contentRect = [window->m_data->view frame];
    // NOTE: The returned location uses base 0,1 not 0,0
    const NSPoint pos        = [event locationInWindow];
    return PointF(pos.x, contentRect.size.height - pos.y);
}

- (void)mouseDown:(NSEvent*)event {
    std::ignore = window->mouseEvent(MouseButton::Left, MouseAction::Press, getKeyMods(event.modifierFlags),
                                     [self eventToPos:event]);
}

- (void)mouseDragged:(NSEvent*)event {
    [self mouseMoved:event];
}

- (void)mouseUp:(NSEvent*)event {
    std::ignore = window->mouseEvent(MouseButton::Left, MouseAction::Release, getKeyMods(event.modifierFlags),
                                     [self eventToPos:event]);
}

- (void)mouseMoved:(NSEvent*)event {
    std::ignore = window->mouseMove([self eventToPos:event]);
}

- (void)rightMouseDown:(NSEvent*)event {
    std::ignore = window->mouseEvent(MouseButton::Right, MouseAction::Press, getKeyMods(event.modifierFlags),
                                     [self eventToPos:event]);
}

- (void)rightMouseDragged:(NSEvent*)event {
    [self mouseMoved:event];
}

- (void)rightMouseUp:(NSEvent*)event {
    std::ignore = window->mouseEvent(MouseButton::Right, MouseAction::Release,
                                     getKeyMods(event.modifierFlags), [self eventToPos:event]);
}

- (void)otherMouseDown:(NSEvent*)event {
    std::ignore = window->mouseEvent(MouseButton((int)[event buttonNumber]), MouseAction::Press,
                                     getKeyMods(event.modifierFlags), [self eventToPos:event]);
}

- (void)otherMouseDragged:(NSEvent*)event {
    [self mouseMoved:event];
}

- (void)otherMouseUp:(NSEvent*)event {
    std::ignore = window->mouseEvent(MouseButton((int)[event buttonNumber]), MouseAction::Release,
                                     getKeyMods(event.modifierFlags), [self eventToPos:event]);
}

- (void)mouseExited:(NSEvent*)event {
    window->mouseEnterOrLeave(false);
}

- (void)mouseEntered:(NSEvent*)event {
    window->mouseEnterOrLeave(true);
}

- (void)viewDidChangeBackingProperties {
    const NSRect contentRect = [window->m_data->view frame];
    const NSRect fbRect      = [window->m_data->view convertRectToBacking:contentRect];
    const float scale =
        std::max(fbRect.size.width / contentRect.size.width, fbRect.size.height / contentRect.size.height);

    if (scale != window->m_data->scale) {
        // if (window->ns.scaleFramebuffer && window->ns.layer)
        // [window->ns.layer setContentsScale:[window->m_data->window backingScaleFactor]];

        window->m_data->scale = scale;
        window->contentScaleChanged(scale, scale);
    }

    if (Brisk::Size(fromNSSize(fbRect.size)) != window->m_framebufferSize) {
        window->m_framebufferSize = fromNSSize(fbRect.size);
        window->windowResized(window->m_windowSize, window->m_framebufferSize);
    }
}

- (void)drawRect:(NSRect)rect {
    // Repaint
}

- (void)updateTrackingAreas {
    if (trackingArea != nil) {
        [self removeTrackingArea:trackingArea];
        trackingArea = nil;
    }

    const NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow |
                                          NSTrackingEnabledDuringMouseDrag | NSTrackingCursorUpdate |
                                          NSTrackingInVisibleRect | NSTrackingAssumeInside;

    trackingArea                        = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                                                       options:options
                                                                         owner:self
                                                                      userInfo:nil];

    [self addTrackingArea:trackingArea];
    [super updateTrackingAreas];
}

- (void)keyDown:(NSEvent*)event {
    const KeyCode key = scanCodeToKeyCode(event.keyCode);

    std::ignore = window->keyEvent(key, [event keyCode], KeyAction::Press, getKeyMods(event.modifierFlags));

    [self interpretKeyEvents:@[ event ]];
}

// Translate a keycode to a Cocoa modifier flag
//
static NSUInteger translateKeyToModifierFlag(KeyCode key) {
    switch (key) {
    case KeyCode::LeftShift:
    case KeyCode::RightShift:
        return NSEventModifierFlagShift;
    case KeyCode::LeftControl:
    case KeyCode::RightControl:
        return NSEventModifierFlagControl;
    case KeyCode::LeftAlt:
    case KeyCode::RightAlt:
        return NSEventModifierFlagOption;
    case KeyCode::LeftSuper:
    case KeyCode::RightSuper:
        return NSEventModifierFlagCommand;
    case KeyCode::CapsLock:
        return NSEventModifierFlagCapsLock;
    default:
        return 0;
    }
}

- (void)flagsChanged:(NSEvent*)event {
    KeyAction action;
    const unsigned int modifierFlags = [event modifierFlags] & NSEventModifierFlagDeviceIndependentFlagsMask;
    const KeyCode key                = scanCodeToKeyCode(event.keyCode);
    const NSUInteger keyFlag         = translateKeyToModifierFlag(key);

    if (keyFlag & modifierFlags) {
        if (window->m_keyState[+key])
            action = KeyAction::Release;
        else
            action = KeyAction::Press;
    } else {
        action = KeyAction::Release;
    }

    std::ignore = window->keyEvent(key, [event keyCode], action, getKeyMods(event.modifierFlags));
}

- (void)keyUp:(NSEvent*)event {
    const KeyCode key = scanCodeToKeyCode(event.keyCode);
    std::ignore = window->keyEvent(key, [event keyCode], KeyAction::Release, getKeyMods(event.modifierFlags));
}

- (void)scrollWheel:(NSEvent*)event {
    double deltaX = [event scrollingDeltaX];
    double deltaY = [event scrollingDeltaY];

    if ([event hasPreciseScrollingDeltas]) {
        deltaX *= 0.1;
        deltaY *= 0.1;
    }

    if (fabs(deltaX) > 0.0 || fabs(deltaY) > 0.0) {
        std::ignore = window->wheelEvent(deltaX, deltaY);
    }
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
    // HACK: We don't know what to say here because we don't know what the
    //       application wants to do with the paths
    return NSDragOperationGeneric;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
    const NSRect contentRect = [window->m_data->view frame];
    // NOTE: The returned location uses base 0,1 not 0,0
    const NSPoint pos        = [sender draggingLocation];
    std::ignore = window->mouseMove(PointF(pos.x, contentRect.size.height - pos.y) * window->m_scale);

    NSPasteboard* pasteboard = [sender draggingPasteboard];
    NSDictionary* options    = @{ NSPasteboardURLReadingFileURLsOnlyKey : @YES };
    NSArray* urls            = [pasteboard readObjectsForClasses:@[ [NSURL class] ] options:options];
    const NSUInteger count   = [urls count];
    if (count) {
        std::vector<std::string> paths;
        for (NSUInteger i = 0; i < count; i++)
            paths.push_back([urls[i] fileSystemRepresentation]);

        std::ignore = window->filesDropped(std::move(paths));
    }

    return YES;
}

- (BOOL)hasMarkedText {
    return [markedText length] > 0;
}

- (NSRange)markedRange {
    if ([markedText length] > 0)
        return NSMakeRange(0, [markedText length] - 1);
    else
        return kEmptyRange;
}

- (NSRange)selectedRange {
    return kEmptyRange;
}

- (void)setMarkedText:(id)string
        selectedRange:(NSRange)selectedRange
     replacementRange:(NSRange)replacementRange {
    markedText = nil;
    if ([string isKindOfClass:[NSAttributedString class]])
        markedText = [[NSMutableAttributedString alloc] initWithAttributedString:string];
    else
        markedText = [[NSMutableAttributedString alloc] initWithString:string];
}

- (void)unmarkText {
    [[markedText mutableString] setString:@""];
}

- (NSArray*)validAttributesForMarkedText {
    return [NSArray array];
}

- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range
                                               actualRange:(NSRangePointer)actualRange {
    return nil;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point {
    return 0;
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
    const NSRect frame = [window->m_data->view frame];
    return NSMakeRect(frame.origin.x, frame.origin.y, 0.0, 0.0);
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {
    NSString* characters;
    NSEvent* event          = [NSApp currentEvent];
    const KeyModifiers mods = getKeyMods(event.modifierFlags);
    const int plain         = !(mods && KeyModifiers::Super);

    if ([string isKindOfClass:[NSAttributedString class]])
        characters = [string string];
    else
        characters = (NSString*)string;

    NSRange range = NSMakeRange(0, [characters length]);
    while (range.length) {
        uint32_t codepoint = 0;

        if ([characters getBytes:&codepoint
                       maxLength:sizeof(codepoint)
                      usedLength:NULL
                        encoding:NSUTF32StringEncoding
                         options:0
                           range:range
                  remainingRange:&range]) {
            if (codepoint >= 0xf700 && codepoint <= 0xf7ff)
                continue;

            std::ignore = window->charEvent(codepoint, false);
        }
    }
}

- (void)doCommandBySelector:(SEL)selector {
}

@end

@interface BriskWindow : NSWindow {
}
@end

@implementation BriskWindow

- (BOOL)canBecomeKeyWindow {
    // Required for NSWindowStyleMaskBorderless windows
    return YES;
}

- (BOOL)canBecomeMainWindow {
    return YES;
}

@end

// Transforms a y-coordinate between the CG display and NS screen spaces
//
static CGFloat transformYCocoa(CGFloat y) {
    return CGDisplayBounds(CGMainDisplayID()).size.height - y - 1;
}

namespace Brisk {

bool PlatformWindow::createWindow() {
    Size size        = max(m_windowSize, Size{ 1, 1 });
    Point initialPos = m_position;
    bool topLevel    = m_data->parent == nil;

    NSRect contentRect;

    if (initialPos.x == dontCare || initialPos.y == dontCare) {
        contentRect = NSMakeRect(0, 0, size.width, size.height);
    } else {
        const int xpos = initialPos.x;
        const int ypos = transformYCocoa(initialPos.y + size.height - 1);
        contentRect    = NSMakeRect(xpos, ypos, size.width, size.height);
    }

    if (topLevel) {
        m_data->delegate = [[BriskWindowDelegate alloc] initWithWindow:this];
        if (m_data->delegate == nil) {
            return false;
        }

        NSUInteger styleMask = NSWindowStyleMaskMiniaturizable;

        if (m_windowStyle && WindowStyle::Undecorated) {
            styleMask |= NSWindowStyleMaskBorderless;
        } else {
            styleMask |= (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable);

            if (m_windowStyle && WindowStyle::Resizable)
                styleMask |= NSWindowStyleMaskResizable;
        }

        m_data->window = [[BriskWindow alloc] initWithContentRect:contentRect
                                                        styleMask:styleMask
                                                          backing:NSBackingStoreBuffered
                                                            defer:NO];

        if (m_data->window == nil) {
            BRISK_SOFT_ASSERT_MSG("Cocoa: Failed to create window", false);
            return false;
        }

        if (initialPos.x == dontCare || initialPos.y == dontCare) {
            [m_data->window center];
            m_data->cascadePoint = [m_data->window cascadeTopLeftFromPoint:m_data->cascadePoint];
        }

        if (m_windowStyle && WindowStyle::Resizable) {
            const NSWindowCollectionBehavior behavior =
                NSWindowCollectionBehaviorFullScreenPrimary | NSWindowCollectionBehaviorManaged;
            [m_data->window setCollectionBehavior:behavior];
        } else {
            const NSWindowCollectionBehavior behavior = NSWindowCollectionBehaviorFullScreenNone;
            [m_data->window setCollectionBehavior:behavior];
        }

        if (m_windowStyle && WindowStyle::TopMost)
            [m_data->window setLevel:NSFloatingWindowLevel];

        // if (wndconfig->maximized)
        // [m_data->window zoom:nil];

        // if (strlen(wndconfig->ns.frameName))
        // [m_data->window setFrameAutosaveName:@(wndconfig->ns.frameName)];
    }

    m_data->view = [[BriskView alloc] initWithWindow:this];

    if (topLevel) {
        [m_data->window setContentView:m_data->view];
        [m_data->window makeFirstResponder:m_data->view];
        [m_data->window setTitle:toNSString(m_window->m_title)];
        [m_data->window setDelegate:m_data->delegate];
        [m_data->window setAcceptsMouseMovedEvents:YES];
        [m_data->window setRestorable:NO];

        [m_data->window setTabbingMode:NSWindowTabbingModeDisallowed];
    } else {
        // Set rect for view to contentRect
        [m_data->view setFrame:contentRect];
        if ([m_data->parent isKindOfClass:[NSWindow class]]) {
            NSWindow* window = (NSWindow*)m_data->parent;
            [window setContentView:m_data->view];
            [window makeFirstResponder:m_data->view];
            [window setAcceptsMouseMovedEvents:YES];
        } else { // NSView
            NSView* view = (NSView*)m_data->parent;
            [view addSubview:m_data->view];
        }
    }

    NSRect frame      = m_data->view.frame;
    m_windowSize      = fromNSSize(frame.size);
    m_framebufferSize = fromNSSize([m_data->view convertRectToBacking:frame].size);

    m_scale           = SizeF{ 1.f * m_framebufferSize.width / m_windowSize.width,
                               1.f * m_framebufferSize.height / m_windowSize.height }
                            .longestSide();

    return true;
}

void PlatformWindow::setSizeLimits(Size minSize, Size maxSize) {
    if (m_data->window == nil)
        return;
    @autoreleasepool {
        if (minSize.width == dontCare || minSize.height == dontCare)
            [m_data->window setContentMinSize:NSMakeSize(0, 0)];
        else
            [m_data->window setContentMinSize:NSMakeSize(minSize.width, minSize.height)];

        if (maxSize.width == dontCare || maxSize.height == dontCare)
            [m_data->window setContentMaxSize:NSMakeSize(DBL_MAX, DBL_MAX)];
        else
            [m_data->window setContentMaxSize:NSMakeSize(maxSize.width, maxSize.height)];

    } // autoreleasepool
}

void PlatformWindow::setStyle(WindowStyle windowStyle) {
    if (m_data->window == nil)
        return;
    m_windowStyle = windowStyle;

    @autoreleasepool {

        NSUInteger styleMask = [m_data->window styleMask];
        // decorated
        if (windowStyle && WindowStyle::Undecorated) {
            styleMask |= NSWindowStyleMaskBorderless;
            styleMask &= ~(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable);
        } else {
            styleMask |= (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable);
            styleMask &= ~NSWindowStyleMaskBorderless;
        }

        // resizeable
        if (windowStyle && WindowStyle::Resizable) {
            styleMask |= NSWindowStyleMaskResizable;

            const NSWindowCollectionBehavior behavior =
                NSWindowCollectionBehaviorFullScreenPrimary | NSWindowCollectionBehaviorManaged;
            [m_data->window setCollectionBehavior:behavior];

        } else {
            styleMask &= ~NSWindowStyleMaskResizable;

            const NSWindowCollectionBehavior behavior = NSWindowCollectionBehaviorFullScreenNone;
            [m_data->window setCollectionBehavior:behavior];
        }

        [m_data->window setStyleMask:styleMask];
        [m_data->window makeFirstResponder:m_data->view];

        // topmost
        if (windowStyle && WindowStyle::TopMost)
            [m_data->window setLevel:NSFloatingWindowLevel];
        else
            [m_data->window setLevel:NSNormalWindowLevel];

        // toolWindow window style is not supported on macOS
    } // autoreleasepool
}

void PlatformWindow::setSize(Size size) {
    @autoreleasepool {
        if (m_data->window != nil) {
            NSRect contentRect = [m_data->window contentRectForFrameRect:[m_data->window frame]];

            contentRect.origin.y += contentRect.size.height - size.height;
            contentRect.size = NSMakeSize(size.width, size.height);

            [m_data->window setFrame:[m_data->window frameRectForContentRect:contentRect] display:YES];
        } else {
            NSView* v               = m_data->view;
            const BOOL needsFlip    = ![v isFlipped];

            NSRect frame            = [v frame];
            const CGFloat oldHeight = frame.size.height;

            frame.size              = NSMakeSize(size.width, size.height);

            if (needsFlip) {
                NSView* parent             = [v superview];
                const CGFloat parentHeight = parent ? parent.bounds.size.height : 0;

                frame.origin.y += (oldHeight - size.height);
                if (parent)
                    frame.origin.y = MIN(frame.origin.y, parentHeight - size.height);
            }

            [v setFrame:frame];
        }
    }
}

bool PlatformWindow::cursorInContentArea() const {
    @autoreleasepool {
        if (m_data->window != nil) {
            const NSPoint pos = [m_data->window mouseLocationOutsideOfEventStream];
            return [m_data->view mouse:pos inRect:[m_data->view frame]];
        } else {
            const NSPoint pos = [[m_data->view window] mouseLocationOutsideOfEventStream];
            return [m_data->view mouse:pos inRect:[m_data->view frame]];
        }
    }
}

void PlatformWindow::updateCursorImage() {
    @autoreleasepool {
        if (m_cursor)
            [(NSCursor*)m_cursor->cursor set];
        else
            [[NSCursor arrowCursor] set];
    }
}

void PlatformWindow::setCursor(Cursor cursor) {
    m_cursor = Internal::platformCursors.getCursor(cursor, m_scale);
    @autoreleasepool {
        if (cursorInContentArea())
            updateCursorImage();
    } // autoreleasepool
}

void PlatformWindow::setPosition(Point point) {
    @autoreleasepool {
        if (m_data->window != nil) {
            const NSRect contentRect = [m_data->view frame];
            const CGFloat flippedY   = transformYCocoa(point.y + contentRect.size.height - 1);

            const NSRect dummyRect   = NSMakeRect(point.x, flippedY, 0, 0);

            const NSRect frameRect   = [m_data->window frameRectForContentRect:dummyRect];

            [m_data->window setFrameOrigin:frameRect.origin];
        } else {
            NSView* v            = m_data->view;
            const BOOL needsFlip = ![v isFlipped];

            NSRect frame         = [v frame];

            if (needsFlip) {
                NSView* parent             = [v superview];
                const CGFloat parentHeight = parent ? parent.bounds.size.height : 0;

                frame.origin               = NSMakePoint(point.x, parentHeight - point.y - frame.size.height);
            } else {
                frame.origin = NSMakePoint(point.x, point.y);
            }

            [v setFrame:frame];
        }
    }
}

HiDPIMode hiDPIMode() {
    return HiDPIMode::FramebufferScaling;
}
} // namespace Brisk
