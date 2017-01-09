package test.sprd.helloworld;

import test.sprd.helloworld.plugin.LeavingUtils;
import test.sprd.helloworld.plugin.TestInterfaceUtils;
import test.sprd.helloworld.plugin.PromptUtils;
import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

public class HelloWorldActivity extends Activity {
    private TextView mTextView;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        PromptUtils.getInstance().showPrompt(this);

        setContentView(R.layout.hello_world);
        mTextView = (TextView) findViewById(R.id.text_hello_world);

        TestInterfaceUtils.getInstance(this).test(this);
    }

    public interface TestInterface{
        public static String TEST = "HelloWorldActivity";
        public void testInterface();
    }

    @Override
    public void onBackPressed() {
        if (LeavingUtils.getInstance(this).canIGo(this)) {
            super.onBackPressed();
        }
    }
}
