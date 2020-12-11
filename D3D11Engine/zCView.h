#pragma once
#include "pch.h"
#include "zTypes.h"
#include "HookedFunctions.h"
#include "oCGame.h"

#if BUILD_GOTHIC_2_6_fix
#include "zViewTypes.h"
#endif

class zCView {
public:

	/** Hooks the functions of this Class */
	static void Hook() {
		DWORD dwProtect;

		HookedFunctions::OriginalFunctions.original_zCViewSetMode = (zCViewSetMode)GothicMemoryLocations::zCView::SetMode;

		VirtualProtect( (void*)GothicMemoryLocations::zCView::SetMode, 0x1B9, PAGE_EXECUTE_READWRITE, &dwProtect ); // zCView::SetMode

		// Replace the actual mode-change in zCView::SetMode. Only do the UI-Changes.
		REPLACE_RANGE( GothicMemoryLocations::zCView::REPL_SetMode_ModechangeStart, GothicMemoryLocations::zCView::REPL_SetMode_ModechangeEnd - 1, INST_NOP );

#if BUILD_GOTHIC_2_6_fix
		// .text:007A62A0; void __thiscall zCView::BlitText(zCView * __hidden this)
		XHook( HookedFunctions::OriginalFunctions.original_zCViewBlitText, GothicMemoryLocations::zCView::BlitText, hooked_BlitText );
		XHook( HookedFunctions::OriginalFunctions.original_zCViewPrint, GothicMemoryLocations::zCView::Print, hooked_Print );
#endif
	}

	static void SetMode( int x, int y, int bpp, HWND* window = nullptr ) {
		hook_infunc
			HookedFunctions::OriginalFunctions.original_zCViewSetMode( x, y, bpp, window );
		hook_outfunc
	}

#if BUILD_GOTHIC_2_6_fix
	static void __fastcall hooked_Print( _zCView* thisptr, void* unknwn, int x, int y, const zSTRING& s ) {
		if ( !Engine::GAPI->GetRendererState().RendererSettings.EnableCustomFontRendering ) {
			HookedFunctions::OriginalFunctions.original_zCViewPrint( thisptr, x, y, s );
			return;
		}
		if ( !thisptr->font ) return;
		thisptr->scrollTimer = 0;

		// Instantly blit Viewport/global-screen
		if ( (thisptr->viewID == 1)
			/*|| (thisptr == Engine::GAPI->GetScreen())*/
			|| (thisptr->viewID == 0) ) {
			Engine::GraphicsEngine->DrawString(
				s.ToChar(),
				thisptr->pposx + thisptr->nax(x), thisptr->pposy + thisptr->nay(y),
				thisptr->font, thisptr->fontColor.dword);
		} else {
			// create a textview for later blitting
			thisptr->CreateText(x, y, s);
		}
	}

	static void __fastcall hooked_BlitText( _zCView* thisptr, void* unknwn ) {
		if ( !Engine::GAPI->GetRendererState().RendererSettings.EnableCustomFontRendering ) {
			HookedFunctions::OriginalFunctions.original_zCViewBlitText( thisptr );
			return;
		}

		thisptr->CheckAutoScroll();
		thisptr->CheckTimedText();

		if ( !thisptr->isOpen || !thisptr->maxTextLength ) return;

		int x, y;

		zCList <zCViewText>* textNode = thisptr->textLines.next;
		zCViewText* text = nullptr;
		DWORD fontColor;
		while ( textNode ) {

			text = textNode->data;
			textNode = textNode->next;
			
			if   (text->colored) { fontColor = text->color.dword; }
			else                 { fontColor = thisptr->fontColor.dword;}

			x = thisptr->pposx + thisptr->nax( text->posx );
			// TODO: Remove additional addition if we get the correct char positioning
			y = thisptr->pposy + thisptr->nay( text->posy );

			if ( !thisptr->font ) continue;
			
			Engine::GraphicsEngine->DrawString( text->text.ToChar(), x, y, text->font, fontColor);
		}
	}

#endif

	/** Prints a message to the screen */
	void PrintTimed( int posX, int posY, const zSTRING& strMessage, float time = 3000.0f, DWORD* col = nullptr ) {
		XCALL( GothicMemoryLocations::zCView::PrintTimed );
	}
};