#include <std.h>

#include <emul.h>
#include <gui.h>
#include <resource.h>
#include <vars.h>
#include <engine/utils/util.h>

struct FILEPREVIEWINFO
{
   OPENFILENAME *ofn;
   struct { HWND h; int dx,dy; } list;
   struct { HWND h; } dlg;
   struct { HWND h; } innerdlg;

   void on_resize();
   void on_change() const;

   void preview_trd(const char *filename) const;
   void preview_scl(const char *filename) const;
   void preview(u8 *cat) const;

} file_preview_info;

void FILEPREVIEWINFO::on_resize()
{
   constexpr int dlgbase = 280;
   constexpr int listbase = 163;

   RECT dlgrc; GetWindowRect(dlg.h, &dlgrc);
   list.dy = (dlgrc.bottom - dlgrc.top) - dlgbase + listbase;

   SetWindowPos(list.h, nullptr, 0, 0, list.dx, list.dy,
                  SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE);
}

void FILEPREVIEWINFO::on_change() const
{
   char filename[512];
   const int r = SendMessage(dlg.h, CDM_GETFILEPATH, sizeof(filename), LPARAM(filename));
   SendMessage(list.h, LVM_DELETEALLITEMS, 0, 0);
   if (r < 0 || (GetFileAttributes(filename) & FILE_ATTRIBUTE_DIRECTORY)) return;

   #if 0 // too slow for every file
   TRKCACHE t;
   FDD TestDrive;
   u8 type = what_is(filename);
   if (type < snSCL) return;
   TestDrive.emptydisk();
   if (!TestDrive.read(type)) return;
   #endif

   char *ext = strrchr(filename, '.');
   if (!ext) return;
   ext++;

   if (!stricmp(ext, "trd")) preview_trd(filename);
   if (!stricmp(ext, "scl")) preview_scl(filename);
}

void FILEPREVIEWINFO::preview(u8 *cat) const
{
   ::dlg = innerdlg.h;
   const u8 bas = getcheck(IDC_PREVIEW_BASIC);
   const u8 del = getcheck(IDC_PREVIEW_ERASED);

   int count = 0;
   char fn[10];

   LVITEM item;
   item.mask = LVIF_TEXT;
   item.pszText = fn;

   for (unsigned p = 0; p < 0x800; p += 0x10) {
      if (!cat[p]) break;
      if (!del && cat[p] == 1) continue;
      if (bas && cat[p+8] != 'B') continue;

      memcpy(fn, cat+p, 8); fn[8] = 0;
      item.iItem = count++;
      item.iSubItem = 0;
      item.iItem = SendMessage(list.h, LVM_INSERTITEM, 0, LPARAM(&item));

      fn[0] = cat[p+8]; fn[1] = 0;
      item.iSubItem = 1;
      SendMessage(list.h, LVM_SETITEM, 0, LPARAM(&item));

      sprintf(fn, "%d", cat[p+13]);
      item.iSubItem = 2;
      SendMessage(list.h, LVM_SETITEM, 0, LPARAM(&item));
   }
}

void FILEPREVIEWINFO::preview_trd(const char *filename) const
{
   u8 cat[0x800];
   FILE *ff = fopen(filename, "rb");
   const int sz = fread(cat, 1, 0x800, ff);
   fclose(ff);
   if (sz != 0x800) return;
   preview(cat);
}

void FILEPREVIEWINFO::preview_scl(const char *filename) const
{
   u8 cat[0x800] = { 0 };
   u8 hdr[16];

   const auto ff = fopen(filename, "rb");
   unsigned sz = fread(hdr, 1, 9, ff), count = 0;

   if (sz == 9 && !memcmp(hdr, "SINCLAIR", 8)) {
	   const unsigned max = hdr[8]; sz = max*14;
      const auto cat1 = (u8*)alloca(sz);
      if (fread(cat1, 1, sz, ff) == sz) {
         for (unsigned i = 0; i < sz; i += 14) {
            memcpy(cat+count*0x10, cat1+i, 14);
            count++; if (count == 0x80) break;
         }
      }
   }

   fclose(ff);
   if (count) preview(cat);
}

UINT_PTR CALLBACK PreviewDlgProc(const HWND dlg, const UINT msg, const WPARAM wp, const LPARAM lp)
{
   switch (msg)
   {
      case WM_INITDIALOG:
      {
         file_preview_info.ofn = (OPENFILENAME*)lp;
         file_preview_info.innerdlg.h = dlg;
         file_preview_info.dlg.h = GetParent(dlg);
         file_preview_info.list.h = GetDlgItem(dlg, IDC_PREVIEW_BOX);

         constexpr auto exflags = LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT;
         SendMessage(file_preview_info.list.h, LVM_SETEXTENDEDLISTVIEWSTYLE, exflags, exflags);

         LVCOLUMN size_col;
         size_col.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
         size_col.fmt = LVCFMT_LEFT;

         size_col.cx = 80;
         size_col.pszText = PSTR("Filename");
         SendMessage(file_preview_info.list.h, LVM_INSERTCOLUMN, 0, LPARAM(&size_col));

         size_col.cx = 40;
         size_col.pszText = PSTR("Ext");
         SendMessage(file_preview_info.list.h, LVM_INSERTCOLUMN, 1, LPARAM(&size_col));

         size_col.cx = 50;
         size_col.pszText = PSTR("Size");
         SendMessage(file_preview_info.list.h, LVM_INSERTCOLUMN, 2, LPARAM(&size_col));

         auto fnt = HFONT(GetStockObject(OEM_FIXED_FONT));
         SendMessage(file_preview_info.list.h, WM_SETFONT, WPARAM(fnt), 0);

         RECT rc; GetWindowRect(file_preview_info.list.h, &rc);
         file_preview_info.list.dx = rc.right - rc.left;
         file_preview_info.list.dy = rc.bottom - rc.top;

         break;
      }

      case WM_COMMAND:
         if (LOWORD(wp) == IDC_PREVIEW_BASIC || LOWORD(wp) == IDC_PREVIEW_ERASED)
            file_preview_info.on_change();
         break;

      case WM_SIZE:
         file_preview_info.on_resize();
         break;

      case WM_NOTIFY:
         if (((OFNOTIFY*)lp)->hdr.code == CDN_SELCHANGE)
            file_preview_info.on_change();
         break;
     default: ;
   }
   return 0;
}

int get_snapshot_file_name(OPENFILENAME *ofn, int save)
{
   ofn->Flags |= save? OFN_PATHMUSTEXIST : OFN_FILEMUSTEXIST;
   ofn->Flags |= OFN_HIDEREADONLY | OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_ENABLESIZING;
   ofn->Flags |= OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;

   ofn->hwndOwner = GetForegroundWindow();
   ofn->hInstance = hIn;
   ofn->lpstrTitle = save? "Save Snapshot / Disk / Tape as" : "Load Snapshot / Disk / Tape";

   ofn->lpfnHook          = PreviewDlgProc;
   ofn->lpTemplateName    = MAKEINTRESOURCE(IDD_FILEPREVIEW);
   ofn->lpstrInitialDir   = temp.snap_dir;

   const BOOL res = save? GetSaveFileName(ofn) : GetOpenFileName(ofn);

   if (res)
   {
       strcpy(temp.snap_dir, ofn->lpstrFile);
       char *ptr = strrchr(temp.snap_dir, '\\');
       if (ptr)
        *ptr = 0;
       return res;
   }
   const DWORD errcode = CommDlgExtendedError();
   if (!errcode) return 0;

   color(CONSCLR_ERROR);
   printf("Error while selecting file. Code is 0x%08lX\n", errcode);
   return 0;
}
