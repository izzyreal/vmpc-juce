package nl.izmar.vmpc2000xl;

import static android.view.View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN;
import static android.view.View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION;
import static android.view.View.SYSTEM_UI_FLAG_LAYOUT_STABLE;

import android.os.Build;
import android.os.Bundle;
import android.view.View;

import nl.izmar.rawkeyboardinput.RawKeyboardInputActivity;

public final class VmpcActivity extends RawKeyboardInputActivity
{
    @SuppressWarnings ("deprecation")
    private void initEdgeToEdge()
    {
        if (Build.VERSION.SDK_INT < 35)
        {
            final View decorView = getWindow().getDecorView();
            final int flags = Build.VERSION.SDK_INT < 30
                    ? (  SYSTEM_UI_FLAG_LAYOUT_STABLE
                       | SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                       | SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN)
                    : 0;

            decorView.setSystemUiVisibility (decorView.getSystemUiVisibility() | flags);
        }

        if (30 <= Build.VERSION.SDK_INT)
            getWindow().setDecorFitsSystemWindows (false);

        if (29 <= Build.VERSION.SDK_INT)
        {
            if (Build.VERSION.SDK_INT < 35)
                getWindow().setStatusBarContrastEnforced (false);

            getWindow().setNavigationBarContrastEnforced (false);
        }
    }

    @Override
    protected void onCreate (Bundle savedInstanceState)
    {
        initEdgeToEdge();
        super.onCreate (savedInstanceState);
    }
}
