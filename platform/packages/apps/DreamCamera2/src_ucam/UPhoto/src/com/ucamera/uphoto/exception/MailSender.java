/*
 *
 * Copyright (C) 2010,2012 Thundersoft Corporation
 *
 */
/*
 * send mail to engineer
 */
package com.ucamera.uphoto.exception;

import android.content.Context;
import android.text.TextUtils;
import android.util.Log;
import java.util.HashSet;
import java.util.Properties;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.activation.CommandMap;
import javax.activation.DataHandler;
import javax.activation.FileDataSource;
import javax.activation.MailcapCommandMap;
import javax.mail.BodyPart;
import javax.mail.Message;
import javax.mail.PasswordAuthentication;
import javax.mail.Session;
import javax.mail.Transport;
import javax.mail.internet.InternetAddress;
import javax.mail.internet.MimeBodyPart;
import javax.mail.internet.MimeMessage;
import javax.mail.internet.MimeMultipart;

public class MailSender extends javax.mail.Authenticator {
    private static final String TAG = "MailSender";

    private String user;
    private String password;
    private Session session;
    private HashSet<String> mAttachements = null;

    public MailSender(Context context) {
        this("log@thundersoft.com","logthunderSOFT!@#");
    }

    public MailSender(String user, String password) {
        this.user = user;
        this.password = password;

        Properties props = new Properties();
        props.put("mail.transport.protocol", "smtp");
        props.put("mail.host", "mail.thundersoft.com");
        props.put("mail.smtp.auth", "true");
        props.put("mail.smtp.user", this.user);
        props.put("mail.smtp.password", this.password);

        props.put("mail.smtp.ssl.enable","true");
        props.put("mail.smtp.ssl.port", "465");
        props.put("mail.smtp.ssl.socketFactory.port", "465");
        props.put("mail.smtp.ssl.trust","javax.net.ssl.SSLSocketFactory");
        props.put("mail.smtp.ssl.socketFactory.class", "javax.net.ssl.SSLSocketFactory");
        props.put("mail.smtp.ssl.socketFactory.fallback", "false");

        /*props.put("mail.smtp.port", "465");
        props.put("mail.smtp.socketFactory.port", "465");
        props.put("mail.smtp.socketFactory.class", "javax.net.ssl.SSLSocketFactory");
        props.put("mail.smtp.socketFactory.fallback", "false");*/

        props.setProperty("mail.smtp.quitwait", "false");
        session = Session.getDefaultInstance(props, this);
    }

    protected PasswordAuthentication getPasswordAuthentication() {
        return new PasswordAuthentication(user, password);
    }

    public synchronized void sendMail(String subject, String body, String replyTo) throws Exception {
        this.sendMail(subject,body,"log@thundersoft.com","camera@thundersoft.com",replyTo);
    }

    public synchronized void sendMail(String subject, String body) throws Exception {
        this.sendMail(subject,body,null);
    }

    public synchronized void sendMail(String subject, String body,
            String sender, String recipients) throws Exception {
        this.sendMail(subject, body,sender,recipients,null);
    }

    public synchronized void sendMail(String subject, String body,
            String sender, String recipients, String replyTo) throws Exception {
        try {
            MimeMessage message = new MimeMessage(session);
            message.setSender(new InternetAddress(sender));
            if (isValidEmail(replyTo)) {
                try {
                    InternetAddress replyAddress = new InternetAddress(replyTo);
                    message.setReplyTo(new InternetAddress[] {replyAddress});
                } catch (Throwable t) {
                    Log.w(TAG, "Unable set reply to address.");
                }
            }
            message.setSubject(subject);
            BodyPart messageBodyPart = new MimeBodyPart();
            messageBodyPart.setText(body);

            MailcapCommandMap mc = (MailcapCommandMap) CommandMap.getDefaultCommandMap();
            mc.addMailcap("text/html;; x-java-content-handler=com.sun.mail.handlers.text_html");
            mc.addMailcap("text/xml;; x-java-content-handler=com.sun.mail.handlers.text_xml");
            mc.addMailcap("text/plain;; x-java-content-handler=com.sun.mail.handlers.text_plain");
            mc.addMailcap("multipart/*;; x-java-content-handler=com.sun.mail.handlers.multipart_mixed");
            mc.addMailcap("message/rfc822;; x-java-content-handler=com.sun.mail.handlers.message_rfc822");
            CommandMap.setDefaultCommandMap(mc);

            MimeMultipart multipart = new MimeMultipart();
            multipart.addBodyPart(messageBodyPart);
            if (mAttachements != null) {
                for (String filename: mAttachements) {
                    MimeBodyPart attachement  = new MimeBodyPart();
                    FileDataSource fds= new FileDataSource(filename);
                    attachement.setDataHandler(new DataHandler(fds));
                    attachement.setFileName(fds.getName());
                    multipart.addBodyPart(attachement);
                }
            }
            message.setContent(multipart);
            message.saveChanges();
            if (recipients.indexOf(',') > 0) {
                message.setRecipients(Message.RecipientType.TO,InternetAddress.parse(recipients));
            } else {
                message.setRecipient(Message.RecipientType.TO, new InternetAddress(recipients));
            }
            session.setDebug(false);
            Transport.send(message);
//            new File(UncatchException.LOG_PATH+UncatchException.LOG_NAME).delete();
        } finally {
            if (mAttachements != null) {
                mAttachements.clear();
            }
        }
    }

    public MailSender addAttachment(String filename) throws Exception {
        if (mAttachements == null) {
            mAttachements = new HashSet<String>();
        }
        if (!TextUtils.isEmpty(filename)) {
            mAttachements.add(filename);
        }
        return this;
    }

    private static final String EMAIL_PATTERN = "\\w+([-+.]\\w+)*@\\w+([-.]\\w+)*\\.\\w+([-.]\\w+)*";
    public static boolean isValidEmail(CharSequence input) {
        if (TextUtils.isEmpty(input)) return false;

        Pattern pattern = Pattern.compile(EMAIL_PATTERN);
        Matcher m = pattern.matcher(input);
        return m.matches();
    }
}
