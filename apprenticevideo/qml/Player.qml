import QtQuick 2.4
import QtQml 2.2
import QtQml.Models 2.2
import com.aragog.apprenticevideo 1.0
import '.'
import 'Utils.js' as Utils

Item
{
  id: player
  objectName: "player"

  signal exitFullScreen()
  signal toggleFullScreen()
  signal togglePlayback()
  signal skipToInPoint()
  signal skipToOutPoint()
  signal stepOneFrameForward()
  signal skipForward()
  signal skipBack()

  focus: true;

  Keys.onPressed:
  {
    event.accepted = false;

    if (playlist.view.visible)
    {
      playlist.handle_event_on_key_pressed(event);
    }

    if (event.accepted)
    {
      return;
    }

    if (event.key == Qt.Key_Escape)
    {
      exitFullScreen();
      event.accepted = true;
    }
    else if (event.key == Qt.Key_MediaTogglePlayPause ||
             event.key == Qt.Key_MediaPause ||
             event.key == Qt.Key_MediaPlay ||
             event.key == Qt.Key_MediaStop)
    {
      togglePlayback();
      event.accepted = true;
    }
    else if (event.key == Qt.Key_Home)
    {
      skipToInPoint();
      event.accepted = true;
    }
    else if (event.key == Qt.Key_End)
    {
      skipToOutPoint();
      event.accepted = true;
    }
    else if (event.key == Qt.Key_N ||
             event.key == Qt.Key_Right ||
             event.key == Qt.Key_Down)
    {
      stepOneFrameForward();
      event.accepted = true;
    }
    else if (event.key == Qt.Key_MediaNext ||
             event.key == Qt.Key_Period ||
             event.key == Qt.Key_Greater ||
             event.key == Qt.Key_PageDown)
    {
      skipForward();
      event.accepted = true;
    }
    else if (event.key == Qt.Key_MediaPrevious ||
             event.key == Qt.Key_Comma ||
             event.key == Qt.Key_Less ||
             event.key == Qt.Key_PageUp)
    {
      skipBack();
      event.accepted = true;
    }
  }

  CanvasQuickFbo
  {
    id: renderer
    objectName: "renderer"

    anchors.fill: parent
    anchors.margins: 0

    // flip it right-side-up:
    transform:
    [
      Scale { yScale: -1; },
      Translate { y: renderer.height; }
    ]
  }

  MouseArea
  {
    anchors.fill: parent
    anchors.margins: 0

    onClicked: {
      parent.focus = true;
    }

    onDoubleClicked: {
      toggleFullScreen();
      mouse.accepted = true;
    }
  }

  Playlist
  {
    id: playlist
    objectName: "playlist"
    visible: true

    anchors.margins: 0
    anchors.fill: parent
  }

  Timeline
  {
    id: timeline
    objectName: "timeline"
    visible: true

    anchors.margins: 0
    anchors.top: playlist.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.topMargin: -this.height
    height: (playlist.calc_title_height(24.0, playlist.width) * 3 / 2)
  }

}
