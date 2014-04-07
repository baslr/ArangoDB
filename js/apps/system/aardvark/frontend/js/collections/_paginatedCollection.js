/*jslint indent: 2, nomen: true, maxlen: 100, vars: true, white: true, plusplus: true */
/*global Backbone, window, $, _ */
(function () {

  "use strict";

  window.PaginatedCollection = Backbone.Collection.extend({
    page: 0,
    pagesize: 10,
    totalAmount: 0,

    getPage: function() {
      return this.page + 1;
    },

    setPage: function(counter) {
      if (counter > this.getLastPageNumber()) {
        this.page = this.getLastPageNumber();
        return;
      }
      if (counter < 1) {
        this.page = 0;
        return;
      }
      this.page = counter - 1;
    },

    getLastPageNumber: function() {
      return Math.max(Math.ceil(this.totalAmount / this.pagesize), 1);
    },

    getOffset: function() {
      return this.getPage() * this.pagesize;
    },

    getPageSize: function() {
      return this.pagesize;
    },

    setToFirst: function() {
      this.page = 0;
    },

    setToLast: function() {
      this.setPage(this.getLastPageNumber());
    },

    setToPrev: function() {
      this.setPage(this.getPage() - 1);

    },

    setToNext: function() {
      this.setPage(this.getPage() + 1);
    },

    setTotal: function(total) {
      this.totalAmount = total;
    }

  });
}());
