<?php

use EasyWeChat\Payment\Payment;
use EasyWeChat\Payment\Order;
use EasyWeChat\Payment\Business;
use EasyWeChat\Payment\UnifiedOrder;
use EasyWeChat\Payment\Notify;

//
use EasyWeChat\Payment\API;
use EasyWeChat\Payment\Merchant;
// for response
use EasyWeChat\Support\Collection;

/**
 * Class CommerceWechatpay
 * Main handler class for Commerce wechatpay, you can extend this class
 * and set your own name using hook_commerce_wechatpay_class_name_alter()
 *
 * @see hook_commerce_wechatpay_class_name_alter()
 * @see commerce_wechatpay_get_handler_class()
 *
 */
Class CommerceWechatpay
{

    protected $appId;
    protected $appSecret;
    protected $mchId;
    protected $mchKey;
    protected $jsParm;
    protected $transaction_info;
    protected $notify_obj;

    public function __construct($appId, $appSecret, $mchId, $mchKey)
    {
        $this->appId = $appId;
        $this->appSecret = $appSecret;
        $this->mchId = $mchId;
        $this->mchKey = $mchKey;
        return $this;
    }

    /**
     * @param $order_num
     * @param $oder_name
     * @param $price
     * @param $notify_url
     * @return bool
     */
    public function proceed_qrpay($order_num, $oder_name, $price, $notify_url)
    {

        //var_dump(func_get_args());


        $merchat = new Merchant(['app_id' => $this->appId, 'merchant_id' => $this->mchId, 'key' => $this->mchKey]);
        $api = new API($merchat);

        /**
         * Create order
         */
        $order = new Order([]);
        $order->trade_type = Order::NATIVE;
        $order->body = $oder_name;
        $order->out_trade_no = $this->orderId2outTradeNo($order_num);
        $order->total_fee = $price;    // 单位为 “分”, 字符串类型
        $order->notify_url = $notify_url;

        $response = $api->prepare($order);

        $code_url = $response->get('code_url');

        /**
         * Process Unified order
         */
        /*
        $unifiedOrder = new UnifiedOrder($business, $order);
        $uni_res = $unifiedOrder->getResponse();
        */

        if (empty($code_url)) {
            return false;
        }

        return $code_url;
    }

    /**
     * Generate out trade number from order_id, feel free to extend
     */
    public function orderId2outTradeNo($order_num)
    {
        $out_trade_no = $order_num;
        return $out_trade_no;
    }

    /**
     * Get order_id from out trade number, feel free to extend
     */
    public function outTradeNo2orderId($out_trade_no)
    {
        $order_num = $out_trade_no;
        return $order_num;
    }

    public function getJsParm($order_num, $oder_name, $price = 1, $notify_url, $sendOpenId = TRUE)
    {
        $this->proceed($order_num, $oder_name, $price, $notify_url, $sendOpenId);
        return $this->jsParm;
    }

    /**
     * Prepare all parameters and send to Wechat
     */
    public function proceed($order_num, $oder_name, $price, $notify_url)
    {
        /**
         * Set business
         */
        $business = new Business($this->appId, $this->appSecret, $this->mchId, $this->mchKey);

        /**
         * Get openid
         */
        $auth = new Auth($this->appId, $this->appSecret);

        if (empty($_SESSION['wechat_logged_user'])) {
            $user = $auth->authorize(null, 'snsapi_base', 'STATE');
            $_SESSION['wechat_logged_user'] = $user->all();
        } else {
            $user = $_SESSION['wechat_logged_user'];
        }
        $openid = $user['openid'];


        /**
         * Create order
         */
        $order = new Order();
        $order->body = $oder_name;
        $order->out_trade_no = $this->orderId2outTradeNo($order_num);
        $order->total_fee = $price;    // 单位为 “分”, 字符串类型
        $order->openid = $openid;
        $order->notify_url = $notify_url;

        /**
         * Process Unified order
         */
        $unifiedOrder = new UnifiedOrder($business, $order);

        /**
         * Generate payment parameters
         */
        $payment = new Payment($unifiedOrder);
        $this->jsParm = $payment->getConfig();

    }

    /**
     * Receive notification and verify it
     *
     * @return bool|array
     */
    public function receiveNotify()
    {

        $notify_instance = $this->getNotifyInstance();
        //$transaction = $this->getNotifyInstance()->verify();

        if (!$notify_instance->isValid()) {
            return false;
        }

        $transaction = $notify_instance->getNotify();

        $this->transaction_info = $transaction->all();

        return $transaction->all();
    }

    public function getNotifyInstance()
    {
        if (!$this->notify_obj) {
            $merchant = new Merchant(['app_id' => $this->appId, 'merchant_id' => $this->mchId, 'key' => $this->mchKey]);
            $payment = new Payment($merchant);
            $this->notify_obj = $payment->getNotify();
        }
        return $this->notify_obj;
    }

    public function getTransactionInfo()
    {
        return $this->transaction_info;
    }

    public function getReplyReceived()
    {
        //return $this->getNotifyInstance()->reply();

        return $this->_reply();
    }

    /**
     * 回复消息, 如果不回复, 微信会一直发送请求到notify_url
     *
     * @param string $code
     * @param string $msg
     *
     * @return string
     */
    private function _reply($code = 'SUCCESS', $msg = 'OK') {
        $params = [
            'return_code' => $code,
            'return_msg' => $msg,
        ];

        return XML::build($params);
    }

}