// /////////////////////////////////////////////////////////////////
// /*
//   Esp infoTainment System (EiTS)
//   For More Information: https://github.com/Nischay2312/EiTS
//   Created by Nischay J., 2023
// */
// /////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////
// /*
//   graphics.h
//   Header file that contains all the fancy graphics related functions.
// */
// /////////////////////////////////////////////////////////////////

void drawBatteryIcon(int16_t startX, int16_t startY, float percentage, bool pluggedIn) {
  // Battery icon dimensions
  const int16_t batteryWidth = 40;
  const int16_t batteryHeight = 20;
  const int16_t batteryTipWidth = 5;
  const int16_t batteryTipHeight = 10;

  const int16_t lightning_Padding = 4;
  const int16_t lightning_width = 6;
  
  // Draw main battery rectangle
  gfx->drawRect(startX, startY, batteryWidth, batteryHeight, WHITE);

  // Draw battery tip
  int16_t tipX = startX + batteryWidth;
  int16_t tipY = startY + (batteryHeight - batteryTipHeight) / 2;
  gfx->fillRect(tipX, tipY, batteryTipWidth, batteryTipHeight, WHITE);

  // Determine fill color based on percentage
  uint16_t fillColor;
  if(percentage >= 70) {
    fillColor = GREEN;
  } else if(percentage >= 30) {
    fillColor = YELLOW;
  } else {
    fillColor = RED;
  }
  
  // Fill battery based on percentage
  int16_t fillWidth = (int16_t) (batteryWidth * (percentage / 100.0));
  gfx->fillRect(startX + 1, startY + 1, fillWidth, batteryHeight - 2, fillColor);

  // If plugged in, draw lightning symbol
  if(pluggedIn) {
    //gfx->drawLine(startX + lightning_Padding, startY + batteryHeight/2, startX + batteryWidth/2, startY + lightning_Padding, WHITE);
    //gfx->drawLine(startX + batteryWidth/2, startY + lightning_Padding, startX + batteryWidth/2, startY + batteryHeight - lightning_Padding, WHITE);
    //gfx->drawLine(startX + batteryWidth/2, startY + batteryHeight - lightning_Padding, startX + batteryWidth - lightning_Padding, startY + batteryHeight/2, WHITE);
    
    gfx->drawLine(startX + batteryWidth - (lightning_Padding + lightning_width/2), startY + lightning_Padding, startX + batteryWidth - (lightning_Padding + lightning_width), startY + batteryHeight/2, WHITE);
    gfx->drawLine(startX + batteryWidth - (lightning_Padding + lightning_width), startY + batteryHeight/2, startX + batteryWidth - lightning_Padding, startY + batteryHeight/2, WHITE);
    gfx->drawLine(startX + batteryWidth - lightning_Padding, startY + batteryHeight/2, startX + batteryWidth - (lightning_Padding + lightning_width/2), startY + batteryHeight - lightning_Padding, WHITE);

  }

  // Display percentage value
  char percentStr[6];  // Enough space for "100%" + null terminator
  snprintf(percentStr, sizeof(percentStr), "%d%%", (int) percentage);
  gfx->setTextColor(percentage > 60 ? BLACK : WHITE);
  gfx->setCursor(startX + 10, startY + 5);
  gfx->print(percentStr);
}